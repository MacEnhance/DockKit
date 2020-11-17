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
#include <sys/mman.h>
#include "SymRez.h"

 __attribute__((constructor))
 static void ctor() {
     
     // Swizzle [NSBundle bundleIdentifier] to not return 'com.apple.dock'
     IMP ogBID = class_replaceMethod(objc_getClass("NSBundle"),
                                     sel_registerName("bundleIdentifier"),
                                     method_getImplementation((Method)^{ return CFSTR("Xom.apple.dock"); }),
                                     NULL);

     // dlopen AppKit which will call [NSApplication load], which is where our problem lies
     dlopen("/System/Library/Frameworks/AppKit.framework/Versions/C/AppKit", RTLD_LAZY);
     
     // Replace the OG implementation because leaving that swizzle WILL cause more problems down the road
     class_replaceMethod(objc_getClass("NSBundle"),
                         sel_registerName("bundleIdentifier"),
                         ogBID,
                         NULL);
     
     
     symrez_t sr_appkit = symrez_new("AppKit");
     if(!sr_appkit) {
         // Should probably handle this more elegantly but 🤷🏻‍♂️
         return;
     }

     /*
      Pseudo code:
      void coreDragRegisterIfNeeded() {
          if (gDidRegisterCoreDrag == false) {
                  gDidRegisterCoreDrag = true;
                  CoreDragRegisterClientInModes([NSApp contextID], _NSAllKitModes());
                  CoreDragSetCGEventProcs(NSCoreDragCGEventBlockingProc, 0);
          }
      }
      
      Patching the function after AppKit is loaded seems to work. As far as I can tell it isn't
      called by any constructors (C or Objc-C +load methods).  If we need to patch the function
      before AppKit loads, it would be simple enough to NOP out CoreDragRegisterClientInModes and
      CoreDragSetCGEventProcs before the call to dlopen. Just not sure if letting gDidRegisterCoreDrag
      get set to true would have any adverse effects given the callbacks aren't being registered...
      */
     void *_coreDragRegisterIfNeeded = sr_resolve_symbol(sr_appkit, "_coreDragRegisterIfNeeded");
     long pagesize = sysconf(_SC_PAGESIZE);
     void *address = (void *)((long)_coreDragRegisterIfNeeded & ~(pagesize - 1));
     mprotect(address, 4096, PROT_READ | PROT_WRITE | PROT_EXEC);
     *(uint8_t *)_coreDragRegisterIfNeeded = 0xc3; //0xc3 = return;
     
     free(sr_appkit);
 }

