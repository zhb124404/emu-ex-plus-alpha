ifeq ($(ENV), android)
 ifeq ($(android_minSDK), 9)
  include $(imagineSrcDir)/resource2/font/android.mk
 else
  include $(imagineSrcDir)/resource2/font/freetype.mk
 endif
else ifeq ($(ENV), ios)
 include $(imagineSrcDir)/resource2/font/uikit.mk
else
 include $(imagineSrcDir)/resource2/font/freetype.mk
endif
