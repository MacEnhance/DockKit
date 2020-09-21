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

 __attribute__((constructor))
 static void ctor() {
     
     // Swizzle [NSBundle bundleIdentifier] to not return 'com.apple.dock'
     IMP ogBID = class_replaceMethod(objc_getClass("NSBundle"),
                         sel_registerName("bundleIdentifier"),
                         method_getImplementation((Method)^{ return CFSTR("com.apple.dick"); }),
                         NULL);
     
     // dlopen AppKit which will call [NSApplication load], which is where our problem lies
     dlopen("/System/Library/Frameworks/AppKit.framework/Versions/C/AppKit", RTLD_LAZY);
     
     // Replace the OG implementation because leaving that swizzle WILL cause more problems down the road
     class_replaceMethod(objc_getClass("NSBundle"),
                         sel_registerName("bundleIdentifier"),
                         ogBID,
                         NULL);
 }

