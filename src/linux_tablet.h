// Instructions: Add -lXi to compiler options to link XInput
// TODO: Add min pressure to Linux if min pressure can be queried on Windows
#pragma once

#include <stdint.h>

#include <X11/extensions/XInput.h>

struct EasyTabInfo
{
    XDevice* Device;
    uint32_t MotionType;
    XEventClass EventClasses[1024];
    uint32_t NumEventClasses;

    int32_t PosX, PosY;
    float Pressure;

    int32_t RangeX, RangeY;
    int32_t MaxPressure;
};


bool EasyTab_Load(EasyTabInfo* EasyTab, Display* Disp)
{
    int32_t Count;
    XDeviceInfoPtr Devices = (XDeviceInfoPtr)XListInputDevices(Disp, &Count);
    if (!Devices) { return false; }

    for (int32_t i = 0; i < Count; i++)
    {
        if (!strstr(Devices[i].name, "stylus") &&
            !strstr(Devices[i].name, "eraser")) { continue; }

        EasyTab->Device = XOpenDevice(Disp, Devices[i].id);
        XAnyClassPtr ClassPtr = Devices[i].inputclassinfo;

        for (int32_t j = 0; j < Devices[i].num_classes; j++)
        {
            switch (ClassPtr->c_class) // NOTE: Most examples use ClassPtr->class, but that gives me the error:
                                       // expected unqualified-id before ‘class’ switch (ClassPtr->class)
                                       //                                                          ^
                                       // Looking at Line 759 of XInput.h, this is expected,
                                       // since 'c_class' is defined in stead of 'class' if the flag '__cplusplus' is defined.
                                       // Not sure why examples compile using C++. Maybe said examples use clang?
            {
                case ValuatorClass:
                {
                    XValuatorInfo *Info = (XValuatorInfo *)ClassPtr;
                    // X
                    if (Info->num_axes > 0)
                    {
                        int32_t min     = Info->axes[0].min_value;
                        EasyTab->RangeX = Info->axes[0].max_value;
                        //printf("Max/min x values: %d, %d\n", min, EasyTab->RangeX); // TODO: Platform-print macro
                    }

                    // Y
                    if (Info->num_axes > 1)
                    {
                        int32_t min     = Info->axes[1].min_value;
                        EasyTab->RangeY = Info->axes[1].max_value;
                        //printf("Max/min y values: %d, %d\n", min, EasyTab->RangeY);
                    }

                    // Pressure
                    if (Info->num_axes > 2)
                    {
                        int32_t min          = Info->axes[2].min_value;
                        EasyTab->MaxPressure = Info->axes[2].max_value;
                        //printf("Max/min pressure values: %d, %d\n", min, EasyTab->MaxPressure);
                    }

                    XEventClass EventClass;
                    DeviceMotionNotify(EasyTab->Device, EasyTab->MotionType, EventClass);
                    if (EventClass)
                    {
                        EasyTab->EventClasses[EasyTab->NumEventClasses] = EventClass;
                        EasyTab->NumEventClasses++;
                    }
                } break;
            }

            ClassPtr = (XAnyClassPtr) ((uint8_t*)ClassPtr + ClassPtr->length); // TODO: Access this as an array to avoid pointer arithmetic?
        }

        XSelectExtensionEvent(Disp, DefaultRootWindow(Disp), EasyTab->EventClasses, EasyTab->NumEventClasses);
    }

    XFreeDeviceList(Devices);

    if (EasyTab->Device != 0) { return true; }
    else                      { return false; }
}
