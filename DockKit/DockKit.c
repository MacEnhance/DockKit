//
//  DockKit.m
//  DockKit
//
//  Created by Jeremy on 9/17/20.
//

#include "DockKit.h"
#include <CoreFoundation/CoreFoundation.h>
#include <objc/objc-runtime.h>
#include <dlfcn.h>
#include "symrez.h"
#include "subhook.h"

extern int pthread_key_init_np(int key, void (*destructor)(void *));

 void NSApplication_load() {
     symrez_t sr_appkit = symrez_new("AppKit");
     if(!sr_appkit)
         return;
     
     void *____NSAppKitFinalizeThreadData = sr_resolve_symbol(sr_appkit, "___NSAppKitFinalizeThreadData");
     
     if(!____NSAppKitFinalizeThreadData)
         return;
     
     pthread_key_init_np(0x42, ____NSAppKitFinalizeThreadData);
 }

 __attribute__((constructor))
 static void ctor() {
     
     // Swizzle [NSBundle bundleIdentifier] to not return 'com.apple.dock'
     IMP ogBID = class_replaceMethod(objc_getClass("NSBundle"),
                         sel_registerName("bundleIdentifier"),
                         method_getImplementation((Method)^{ return CFSTR("com.apple.dick"); }),
                         NULL);
     
     // dlopen AppKit which will call [NSApplication load], which is where our problem lies
     void *h = dlopen("/System/Library/Frameworks/AppKit.framework/Versions/C/AppKit", RTLD_LAZY);
     
     // Replace the OG implementation because leaving that swizzle WILL cause more problems down the road
     class_replaceMethod(objc_getClass("NSBundle"),
                         sel_registerName("bundleIdentifier"),
                         ogBID,
                         NULL);
     
     // Grab a pointer to [NSApplication load]
     Method origLoad = class_getClassMethod(objc_getClass("NSApplication"), sel_getUid("load"));
     IMP origLoadImp = method_getImplementation(origLoad);
     
     // Replace -load with a rewrite of the funciton without the check for bundleIdentifier
     subhook_t sh_nsAppLoad = subhook_new((void*)origLoadImp, NSApplication_load, SUBHOOK_64BIT_OFFSET);
     subhook_install(sh_nsAppLoad);

     dlclose(h);
 }

