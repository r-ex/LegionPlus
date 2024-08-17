#include "pch.h"
#include "RadAudioDecoder.h"

#include "thirdparty/include/rada_decode.h"
#include <MilesLib.h>

__int64 ASI_stream_parse_metadata(const char* buffer, size_t dataSize, uint16_t* outChannels, uint32_t* outSampleRate, uint32_t* outFrameCount, int* outSizeInfo, uint32_t* outMemNeededToOpen)
{
	const RadAFileHeader* header = RadAGetFileHeader(reinterpret_cast<const uint8_t*>(buffer), dataSize);

	if (!header)
		return 0;

	uint32_t sizeToOpen = 0;

	if (RadAGetMemoryNeededToOpen(reinterpret_cast<const uint8_t*>(buffer), dataSize, &sizeToOpen))
		return 0;

	if (header->frame_count > UINT32_MAX
		|| header->file_size > UINT32_MAX)
		return 0;

	uint32_t sampleRate = 0;
	switch (header->sample_rate)
	{
	case ERadASampleRate::Rate_24000:
		sampleRate = 24000;
		break;
	case ERadASampleRate::Rate_32000:
		sampleRate = 32000;
		break;
	case ERadASampleRate::Rate_44100:
		sampleRate = 44100;
		break;
	case ERadASampleRate::Rate_48000:
		sampleRate = 48000;
		break;
	}

	*outChannels = header->channels;
	*outSampleRate = sampleRate;
	*outFrameCount = header->frame_count;
	
	outSizeInfo[0] = sizeToOpen; // container data size required to open the decoder stream
	outSizeInfo[1] = header->max_block_size;
	outSizeInfo[2] = RadADecodeBlock_MaxOutputFrames; // max number of samples per decode
	outSizeInfo[3] = 2; // decode format, 0 - pcm, 2 - 32bit float

	if(outMemNeededToOpen)
		*outMemNeededToOpen = sizeToOpen;

	return 1;
}

typedef uint32_t(*ASI_read_stream_f)(char* buffer, uint32_t readAmount, void* userData);

int ASI_open_stream(RadAContainer* container, size_t* pContainerSize, ASI_read_stream_f readFunc, void* streamUserData)
{
	char* headerBuffer = new char[sizeof(RadAFileHeader)];

	if (readFunc(headerBuffer, sizeof(RadAFileHeader), streamUserData) != sizeof(RadAFileHeader))
		return 0;

	const RadAFileHeader* header = RadAGetFileHeader(reinterpret_cast<const uint8_t*>(headerBuffer), sizeof(RadAFileHeader));

	if (!header)
		return 0;

	const size_t v11 = RadAGetBytesToOpen(header);

	if (v11 == 0 || v11 < sizeof(RadAFileHeader))
		return 0;


	char* fileBuffer = new char[v11];
	memcpy(fileBuffer, headerBuffer, sizeof(RadAFileHeader));

	// clean up header buffer
	delete[] headerBuffer;
	headerBuffer = nullptr;
	
	if (readFunc(fileBuffer + sizeof(RadAFileHeader), v11 - sizeof(RadAFileHeader), streamUserData) != v11 - sizeof(RadAFileHeader))
		return 0;

	if (!RadAOpenDecoder(reinterpret_cast<const uint8_t*>(fileBuffer), v11, container, *pContainerSize))
		return 0;
	
	if (header->seek_table_entry_count != 0)
		return 2;
	else
		return 1;
}

void ASI_notify_seek(void* container)
{
	RadANotifySeek(reinterpret_cast<RadAContainer*>(container));
}

size_t ASI_stream_seek_to_frame(RadAContainer* container, size_t targetFrame, size_t* outFrameAtLocation, uint32_t* a4)
{
	const RadAFileHeader* header = RadAGetFileHeaderFromContainer(container);

	if (header->seek_table_entry_count == 0)
		return 0;

	size_t frameAtLocation = 0;
	size_t frameBlockSize = 0;
	const size_t res = RadASeekTableLookup(container, targetFrame, &frameAtLocation, &frameBlockSize);

	if (res == 0)
		return 0;

	if (frameAtLocation > UINT32_MAX)
		return 0;

	if (frameBlockSize > INT32_MAX)
		return 0;

	if (outFrameAtLocation)
		*outFrameAtLocation = frameAtLocation;

	if (a4)
	{
		if (targetFrame == -1)
			*a4 = header->file_size - res;
		else
			*a4 = frameBlockSize;
	}

	return res;
}

size_t ASI_stream_direct_seek(const char* fileBuffer, size_t fileSize, size_t targetFrame, size_t* outFrameAtLocation, size_t* a5)
{
	size_t frameAtLocation = 0;
	size_t frameBlockSize = 0;
	const size_t res = RadADirectSeekTableLookup(reinterpret_cast<const uint8_t*>(fileBuffer), fileSize, targetFrame, &frameAtLocation, &frameBlockSize);

	if (res == 0)
		return 0;

	if (frameAtLocation > UINT32_MAX)
		return 0;

	if (frameBlockSize > INT32_MAX)
		return 0;

	*outFrameAtLocation = frameAtLocation;
	*a5 = frameBlockSize;

	return res;
}

void ASI_decode_block(RadAContainer* container, const char* inputBuffer, size_t inputBufferLen, char* outputBuffer, size_t outputBufferLen, uint32_t* outInputBytesUsed, uint32_t* outFramesDecoded)
{
	RadAExamineBlockResult res = RadAExamineBlock(container, reinterpret_cast<const uint8_t*>(inputBuffer), inputBufferLen, 0);

	// Handle unsuccessful results
	switch (res)
	{
	case RadAExamineBlockResult::Incomplete:
	{
		*outInputBytesUsed = 0;
		*outFramesDecoded = 0;
		return;
	}
	case RadAExamineBlockResult::Invalid:
	{
		*outInputBytesUsed = min(inputBufferLen, 4);
		*outFramesDecoded = 0;
		return;
	}
	}

	const RadAFileHeader* header = RadAGetFileHeaderFromContainer(container);

	const size_t numOutputFloatsPerChannel = outputBufferLen / (4ull * header->channels);

	size_t consumedBytes = 0;

	short numFramesDecoded = RadADecodeBlock(container, reinterpret_cast<const uint8_t*>(inputBuffer), inputBufferLen, reinterpret_cast<float*>(outputBuffer), numOutputFloatsPerChannel, &consumedBytes);

	if (numFramesDecoded == -1)
	{
		*outInputBytesUsed = 0;
		*outFramesDecoded = 0;
		return;
	}

	// Error
	if (numFramesDecoded == -2)
	{
		*outInputBytesUsed = min(inputBufferLen, 4);
		*outFramesDecoded = 0;
		return;
	}

	*outInputBytesUsed = consumedBytes;
	*outFramesDecoded = numFramesDecoded;
}

void ASI_get_block_size(RadAContainer* container, const char* inputBuffer, size_t inputBufferLen, uint32_t* a4, uint32_t* blockSize, uint32_t* a6)
{
	uint32_t bufferSizeNeeded = 0;
	RadAExamineBlockResult res = RadAExamineBlock(container, reinterpret_cast<const uint8_t*>(inputBuffer), inputBufferLen, &bufferSizeNeeded);

	if (res == RadAExamineBlockResult::Invalid)
	{
		*a4 = 0;
		*blockSize = 0xFFFF;
		*a6 = 0;
	}
	else if (res == RadAExamineBlockResult::Incomplete || res == RadAExamineBlockResult::Valid)
	{
		*a4 = 0;
		*blockSize = bufferSizeNeeded;
		*a6 = 9;
	}
}

void* GetRadAudioDecoder()
{
	MilesASIDecoder* radAudio = new MilesASIDecoder;

	memset(radAudio, 0, sizeof(MilesASIDecoder));

	radAudio->unk0 = 1;
	radAudio->decoderType = 6;
	radAudio->ASI_stream_parse_metadata = ASI_stream_parse_metadata;
	radAudio->ASI_open_stream = ASI_open_stream;
	radAudio->ASI_notify_seek = ASI_notify_seek;
	radAudio->ASI_stream_seek_to_frame = ASI_stream_seek_to_frame;
	radAudio->ASI_stream_seek_direct = ASI_stream_direct_seek;
	radAudio->ASI_decode_block = ASI_decode_block;
	radAudio->ASI_get_block_size = ASI_get_block_size;

	return radAudio;
}