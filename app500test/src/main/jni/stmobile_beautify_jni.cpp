#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <st_mobile_beautify.h>
#include "utils.h"
#include <cstring>
#define  LOG_TAG    "STBeautifyNative"

extern "C" {
    JNIEXPORT jint JNICALL Java_com_sensetime_stmobile_STBeautifyNative_createInstance(JNIEnv * env, jobject obj);
    JNIEXPORT jint JNICALL Java_com_sensetime_stmobile_STBeautifyNative_setParam(JNIEnv * env, jobject obj, jint type, jfloat value);
    JNIEXPORT jint JNICALL Java_com_sensetime_stmobile_STBeautifyNative_processBufferNotInGLContext(JNIEnv * env, jobject obj, jbyteArray pInputImage, jint informat,
                                    jint outputWidth, jint outputHeight, jobjectArray facesArrayIn, jbyteArray pOutputImage, jint outformat, jobjectArray facesArrayOut);
    JNIEXPORT jint JNICALL Java_com_sensetime_stmobile_STBeautifyNative_processBufferInGLContext(JNIEnv * env, jobject obj,jbyteArray pInputImage, jint informat, jint outputWidth, jint outputHeight,
                                            jobjectArray facesArrayIn, jbyteArray pOutputImage, jint outformat, jobjectArray facesArrayOut);
    JNIEXPORT jint JNICALL Java_com_sensetime_stmobile_STBeautifyNative_processTexture(JNIEnv * env, jobject obj,jint textureIn,
                                jint outputWidth, jint outputHeight, jobjectArray facesArrayIn, jint textureOut, jobjectArray facesArrayOut);
    JNIEXPORT jint JNICALL Java_com_sensetime_stmobile_STBeautifyNative_processTextureAndOutputBuffer(JNIEnv * env, jobject obj,jint textureIn,
                                    jint outputWidth, jint outputHeight, jobjectArray facesArrayIn, jint textureOut, jbyteArray outputArray, jint outputFormat, jobjectArray facesArrayOut);
    JNIEXPORT jint JNICALL Java_com_sensetime_stmobile_STBeautifyNative_destroyBeautify(JNIEnv * env, jobject obj);
};

JNIEXPORT jint JNICALL Java_com_sensetime_stmobile_STBeautifyNative_createInstance(JNIEnv * env, jobject obj)
{
    LOGE("createInstance Enter");
    st_handle_t handle;
    int result = (int)st_mobile_beautify_create(&handle);
    if(result != 0)
    {
        LOGE("create handle failed");
        return result;
    }
    setHandle(env, obj, handle);
    LOGE("createInstance Exit");
    return result;
}

JNIEXPORT jint JNICALL Java_com_sensetime_stmobile_STBeautifyNative_setParam(JNIEnv * env, jobject obj, jint type, jfloat value)
{
    st_handle_t handle = getHandle<st_handle_t>(env, obj);
    if(handle == NULL)
    {
        return JNI_FALSE;
    }
    LOGE("set Param for %d, %f", type, value);
    int result = (int)st_mobile_beautify_setparam(handle,(st_beautify_type)type,value);

    return result;
}

void preProcess(JNIEnv *env, jobjectArray facesArrayIn, int &facesArrayLen, st_mobile_106_t **p_faces_106_in,  st_mobile_106_t **p_faces_array_out) {
    if (facesArrayIn != NULL) {
        facesArrayLen = env->GetArrayLength(facesArrayIn);
        //LOGE("facesArrayLen:%d", facesArrayLen);
        *p_faces_array_out =  new st_mobile_106_t[facesArrayLen];
        *p_faces_106_in = new st_mobile_106_t[facesArrayLen];
        for (int i = 0; i < facesArrayLen; ++i) {
            jobject obj = env->GetObjectArrayElement(facesArrayIn, i);
            bool result = convert2mobile_106(env, obj, (*p_faces_106_in)[i]);
            //LOGE("-result: %d", result);
            env->DeleteLocalRef(obj);
        }
    }
}

void afterProcess(JNIEnv *env, int facesArrayLen, st_mobile_106_t *p_faces_array_out, jobjectArray facesArrayOut) {
    if(facesArrayOut != NULL){
            jclass st_mobile_106_class = env->FindClass("com/sensetime/stmobile/model/STMobile106");
        //    facesArrayOut = env->NewObjectArray(facesArrayLen, st_mobile_106_class, 0);
            for (int i = 0; i < facesArrayLen; ++i) {
                jobject faceObj = convert2MobileFace106(env, p_faces_array_out[i]);
                env->SetObjectArrayElement(facesArrayOut, i, faceObj);
                env->DeleteLocalRef(faceObj);
            }
            env->DeleteLocalRef(st_mobile_106_class);
    }
}

JNIEXPORT jint JNICALL Java_com_sensetime_stmobile_STBeautifyNative_processBufferInGLContext(JNIEnv * env, jobject obj,jbyteArray pInputImage,
    jint informat, jint outputWidth, jint outputHeight, jobjectArray facesArrayIn, jbyteArray pOutputImage,
    jint outformat, jobjectArray facesArrayOut)
{
    LOGE("Enter processBuffer");
    st_handle_t handle = getHandle<st_handle_t>(env, obj);

    if(handle == NULL)
    {
        LOGE("processBuffer---handle is null");
        return ST_E_INVALIDARG;
    }

    jbyte *srcdata = (jbyte*) (env->GetByteArrayElements(pInputImage, 0));
    jbyte *dstdata = (jbyte*) env->GetByteArrayElements(pOutputImage, 0);

    st_pixel_format pixel_format = (st_pixel_format)informat;
    int stride = getImageStride(pixel_format, outputWidth);
    st_mobile_106_t *p_faces_array_out = NULL;
    st_mobile_106_t *p_faces_106_in = NULL;
    int facesArrayLen = 0;
    preProcess(env, facesArrayIn, facesArrayLen, &p_faces_106_in, &p_faces_array_out);

    int result = (int)st_mobile_beautify_process_buffer(handle,(unsigned char *)srcdata, (st_pixel_format)pixel_format,
    outputWidth, outputHeight, stride, p_faces_106_in, facesArrayLen, (unsigned char*)dstdata,(st_pixel_format)outformat, p_faces_array_out);


    env->ReleaseByteArrayElements(pInputImage, srcdata, 0);
    env->ReleaseByteArrayElements(pOutputImage, dstdata, 0);

    if (result == ST_OK) {
        afterProcess(env, facesArrayLen, p_faces_array_out, facesArrayOut);
    }

    if (p_faces_106_in) {
        delete[] p_faces_106_in;
    }
    if (p_faces_array_out) {
        delete[] p_faces_array_out;
    }

    LOGE("Exit processBuffer");
    return result;
}

JNIEXPORT jint JNICALL Java_com_sensetime_stmobile_STBeautifyNative_processBufferNotInGLContext(JNIEnv * env, jobject obj,
    jbyteArray pInputImage, jint informat, jint outputWidth, jint outputHeight, jobjectArray facesArrayIn,
    jbyteArray pOutputImage, jint outformat, jobjectArray facesArrayOut)
{
    LOGE("Enter processPicture");
    st_handle_t handle = getHandle<st_handle_t>(env, obj);

    if(handle == NULL)
    {
        LOGE("processBuffer---handle is null");
        return ST_E_INVALIDARG;
    }

    jbyte *srcdata = (jbyte*) (env->GetByteArrayElements(pInputImage, 0));
    jbyte *dstdata = (jbyte*) env->GetByteArrayElements(pOutputImage, 0);

    st_pixel_format pixel_format = (st_pixel_format)informat;
    int stride = getImageStride(pixel_format, outputWidth);
    st_mobile_106_t *p_faces_array_out = NULL;
    st_mobile_106_t *p_faces_106_in = NULL;
    int facesArrayLen = 0;
    preProcess(env, facesArrayIn, facesArrayLen, &p_faces_106_in, &p_faces_array_out);

    int result = (int)st_mobile_beautify_process_picture(handle,(unsigned char *)srcdata, (st_pixel_format)pixel_format, outputWidth, outputHeight, stride,
        p_faces_106_in, facesArrayLen, (unsigned char*)dstdata,(st_pixel_format)outformat,p_faces_array_out);

    if (result == ST_OK) {
        afterProcess(env, facesArrayLen, p_faces_array_out, facesArrayOut);
    }

    if (p_faces_106_in) {
        delete[] p_faces_106_in;
    }
    if (p_faces_array_out) {
        delete[] p_faces_array_out;
    }
    env->ReleaseByteArrayElements(pInputImage, srcdata, 0);
    env->ReleaseByteArrayElements(pOutputImage, dstdata, 0);
    LOGE("Exit processPicture");
    return result;
}

JNIEXPORT jint JNICALL Java_com_sensetime_stmobile_STBeautifyNative_processTexture(JNIEnv * env, jobject obj,jint textureIn, jint outputWidth, jint outputHeight,
jobjectArray facesArrayIn, jint textureOut, jobjectArray facesArrayOut)
{
    LOGI("Enter processTexture, the texture in ID is %d, out: %d",textureIn, textureOut);
    st_handle_t handle = getHandle<st_handle_t>(env, obj);

    if(handle == NULL)
    {
        LOGE("processTexture---handle is null");
        return ST_E_INVALIDARG;
    }

    st_mobile_106_t *p_faces_array_out = NULL;
    st_mobile_106_t *p_faces_106_in = NULL;
    int facesArrayLen = 0;
    preProcess(env, facesArrayIn, facesArrayLen, &p_faces_106_in, &p_faces_array_out);


    int result = (int)st_mobile_beautify_process_texture(handle, textureIn,
            outputWidth, outputHeight, p_faces_106_in, facesArrayLen, textureOut, p_faces_array_out);
    LOGI("Exit processTexture, the result is %d", result);

    if (result == ST_OK && facesArrayOut != NULL) {
        afterProcess(env, facesArrayLen, p_faces_array_out, facesArrayOut);
        LOGI("111facesArrayOut: %x", facesArrayOut);
    }

    if (p_faces_106_in) {
        delete[] p_faces_106_in;
    }
    if (p_faces_array_out) {
        delete[] p_faces_array_out;
    }

    return result;
}

JNIEXPORT jint JNICALL Java_com_sensetime_stmobile_STBeautifyNative_processTextureAndOutputBuffer(JNIEnv * env, jobject obj,jint textureIn,
                                    jint outputWidth, jint outputHeight, jobjectArray facesArrayIn, jint textureOut, jbyteArray outputArray,
                                    jint outputFormat, jobjectArray facesArrayOut)
{

    LOGI("Enter processTextureAndOutputBuffer, the texture in ID is %d, out: %d",textureIn, textureOut);
    st_handle_t handle = getHandle<st_handle_t>(env, obj);

    if(handle == NULL)
    {
        LOGE("processTexture---handle is null");
        return ST_E_INVALIDARG;
    }

    unsigned char *outputInfo = NULL;
    if (outputArray != NULL) {
        outputInfo = (unsigned char*) (env->GetByteArrayElements(outputArray, 0));
    }
    st_mobile_106_t *p_faces_array_out = NULL;
    st_mobile_106_t *p_faces_106_in = NULL;
    int facesArrayLen = 0;
    preProcess(env, facesArrayIn, facesArrayLen, &p_faces_106_in, &p_faces_array_out);

    LOGE("before beautify, w:%d, h:%d", outputWidth, outputHeight);

    int result = (int)st_mobile_beautify_process_and_output_texture(handle, textureIn,
            outputWidth, outputHeight, p_faces_106_in, facesArrayLen, textureOut, outputInfo,
            (st_pixel_format)outputFormat, p_faces_array_out);
    LOGI("Exit processTexture, the result is %d", result);

    if (result == ST_OK) {
        afterProcess(env, facesArrayLen, p_faces_array_out, facesArrayOut);
        LOGI("facesArrayOut: %x", facesArrayOut);
    }

    if (outputArray != NULL) {
        env->ReleaseByteArrayElements(outputArray, (jbyte*)outputInfo, 0);
    }
    if (p_faces_106_in) {
        delete[] p_faces_106_in;
    }
    if (p_faces_array_out) {
        delete[] p_faces_array_out;
    }

    return result;
}

JNIEXPORT jint JNICALL Java_com_sensetime_stmobile_STBeautifyNative_destroyBeautify(JNIEnv * env, jobject obj)
{
    LOGI("Enter destroyBeautify");
    st_handle_t handle = getHandle<st_handle_t>(env, obj);
    if(handle == NULL)
    {
        LOGE("destroyBeautify---handle is null");
        return JNI_FALSE;
    }
    setHandle<st_handle_t>(env, obj, NULL);
    st_mobile_beautify_destroy(handle);
    return JNI_TRUE;
}
