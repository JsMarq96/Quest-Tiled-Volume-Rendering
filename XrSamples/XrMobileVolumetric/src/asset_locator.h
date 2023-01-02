//
// Created by js on 29/12/22.
//

#ifndef OCULUSROOT_ASSET_LOCATOR_H
#define OCULUSROOT_ASSET_LOCATOR_H

#include <stdio.h>
#include <android_native_app_glue.h>
#include <string.h>
#include <android/log.h>
#include <cstdlib>
#include <unistd.h>
#include <sys/stat.h>
#include <zip.h>


//TODO only unzip assets once!
namespace Assets {
    struct sAssetLocator {
        const char* apk_dir;
        char* root_asset_dir;
        uint32_t   root_asset_length = 0;

        void init(JNIEnv *env,
                  ANativeActivity *activity) {
            jboolean isCopy;

            jclass clazz = env->GetObjectClass(activity->clazz);
            jmethodID methodID = env->GetMethodID(clazz,
                                                  "getPackageCodePath",
                                                  "()Ljava/lang/String;");
            jobject result_str = env->CallObjectMethod(activity->clazz,
                                                       methodID);
            apk_dir = env->GetStringUTFChars( (jstring)result_str,
                                              &isCopy);

            // TODO: Dynamically fetch this folder or wherever you are supposed to store it
            root_asset_length = strlen(activity->internalDataPath);
            root_asset_dir = (char*) malloc(strlen(activity->internalDataPath) + 6);
            strcpy((char*) root_asset_dir,
                   activity->internalDataPath);
            strcat((char*) root_asset_dir,
                   "/assets");

            // Test if teh folder where we are going to store the assets exists, and if not, create
            // it with the correct permissions
            mkdir(root_asset_dir,
                  0777);
            if (access(root_asset_dir,
                       F_OK)) {

            }

            root_asset_dir[root_asset_length] = '\0';
        }

        void destroy() {
            //free(apk_dir);
            //free(root_asset_dir);
        }
    };

    inline sAssetLocator* fetch_asset_locator() {
        static sAssetLocator asset_loc;
        return &asset_loc;
    }

    // Wrapper arround asset locator
    inline void get_asset_dir(const char* asset_name,
                              char** asset_dir) {
        sAssetLocator * asset_loc = fetch_asset_locator();
        int err_code;
        struct zip_stat apk_zip_st;
        FILE *dump_file = NULL;

        zip *apk = zip_open(asset_loc->apk_dir,
                            0,
                            &err_code);

        // TODO:Check errcode

        zip_stat_init(&apk_zip_st);
        zip_stat(apk,
                 asset_name,
                 0,
                 &apk_zip_st);

        char* raw_file = (char*) malloc(apk_zip_st.size);

        zip_file *file = zip_fopen(apk,
                                   asset_name,
                                   0);
        zip_fread(file,
                  raw_file,
                  apk_zip_st.size);

        int asset_name_len = strlen(asset_name);

        *asset_dir = (char*) malloc( asset_loc->root_asset_length + asset_name_len + 1);
        strcpy(*asset_dir,
               asset_loc->root_asset_dir);
        strcat(*asset_dir,
               "/");
        strcat(*asset_dir,
               asset_name);
        //*asset_dir[strlen(asset_loc->root_asset_dir) + asset_name_len] = '\0';

        dump_file = fopen(*asset_dir,
                          "wb");

        //char* error = strerror(errno);
        //error[2] = 0;

        assert(dump_file != NULL && "Cannot open file to store asset");

        fwrite(raw_file,
               apk_zip_st.size,
               1,
               dump_file);

        zip_fclose(file);
        fclose(dump_file);
        zip_close(apk);
        free(raw_file);
    }
}

#endif //OCULUSROOT_ASSET_LOCATOR_H
