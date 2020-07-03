#pragma once


#ifdef DLLONE_EXPORTS
#define DLLONE_API __declspec(dllexport)
#else 
#define DLLONE_API __declspec(dllimport)
#endif

#ifdef __cplusplus
extern "C" {
#endif

DLLONE_API int VCamSource_Create(void **source);
DLLONE_API int VCamSource_Destroy(void **source);
DLLONE_API int VCamSource_Init(void *source);
DLLONE_API int VCamSource_FillFrame(void *source, void* buffer, size_t len,
		int width, int height);

DLLONE_API int VMicSource_Create(void **source);
DLLONE_API int VMicSource_Destroy(void **source);
DLLONE_API int VMicSource_Init(void *source);
DLLONE_API int VMicSource_GetFormat(void *source, void **format);
DLLONE_API int VMicSource_StartPlay(void *source);
DLLONE_API int VMicSource_StopPlay(void *source);
DLLONE_API int VMicSource_FillSample(void *source, void* buffer, size_t frameLength);

#ifdef __cplusplus
}
#endif
