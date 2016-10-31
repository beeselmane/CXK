#include <SystemLoader/SystemLoader.h>

#if kCXDebug

typedef struct {
    SLConsole rootConsole;
    SLGraphicsContext *context;
    SLBitmapFont *selectedFont;
    SLConsoleLocation cursor;
    UInt32 baseOffsetX;
    UInt32 baseOffsetY;
    UInt32 color;
    UInt32 backgroundColor;
} SLVideoConsole;

#define kSLVideoConsoleMaxNumber 4

static SLVideoConsole gSLVideoConsoles[kSLVideoConsoleMaxNumber];

void SLVideoConsoleSanitizeCursor(SLVideoConsole *console)
{
    if (console->cursor.x >= console->rootConsole.width)
    {
        console->cursor.x = 0;
        console->cursor.y++;
    }

    if (console->cursor.y >= console->rootConsole.height)
    {
        UInt32 rows = (console->rootConsole.height - console->cursor.y) + 1;

        OSCount blankPixelCount = console->context->width * rows * console->selectedFont->height;
        OSAddress blankOffset = (console->context->framebuffer + console->context->pixelCount) - blankPixelCount;
        OSAddress framebufferOffset = console->context->framebuffer + blankPixelCount;
        OSSize pixelCount = console->context->pixelCount - blankPixelCount;

        CXKMemoryCopy(framebufferOffset, console->context->framebuffer, pixelCount * sizeof(UInt32));
        CXKMemorySetValue(blankOffset, blankPixelCount * sizeof(UInt32), 0x00);

        console->cursor.y -= rows;
    }
}

void SLVideoConsoleOutputCharacter(UInt8 character, SLVideoConsole *console)
{
    SLGraphicsPoint outputLocation;
    outputLocation.y = (console->cursor.y * console->selectedFont->height) + console->baseOffsetY;
    outputLocation.x = (console->cursor.x * console->selectedFont->width ) + console->baseOffsetX;

    //SLDelayProcessor(25000, !SLBootServicesHaveTerminated());

    switch (character)
    {
        case '\n': {
            console->cursor.x = 0;
            console->cursor.y++;
        } break;
        case '\r': {
            console->cursor.x = 0;
        } break;
        case '\b': {
            if (!console->cursor.x) {
                if (console->cursor.y)
                {
                    console->cursor.x = console->rootConsole.width - 1;
                    console->cursor.y--;
                }
            } else {
                console->cursor.x--;
            }
        } break;
        default: {
            SLGraphicsContextWriteCharacter(console->context, character, outputLocation, console->selectedFont, console->color, console->backgroundColor);
            console->cursor.x++;
        } break;
    }

    SLVideoConsoleSanitizeCursor(console);
}

void SLVCOC(UInt8 c)
{
    SLVideoConsoleOutputCharacter(c, &gSLVideoConsoles[0]);
}

void SLVideoConsoleOutput(UInt8 *string, OSSize size)
{
    for (UInt8 i = 0; i < size; i++)
    {
        for (UInt8 j = 0; j < kSLVideoConsoleMaxNumber; j++)
        {
            if (!(~gSLVideoConsoles[j].rootConsole.id))
                return;

            SLVideoConsoleOutputCharacter(string[i], &gSLVideoConsoles[j]);
        }
    }
}

UInt8 *SLVideoConsoleInput(OSSize *size, UInt8 finalCharacter)
{
    return kOSNullPointer;
}

void SLVideoConsoleMoveCursor(SLConsoleLocation location)
{
    //
}

SLConsoleLocation SLVideoConsoleGetCursor(void)
{
    return gSLVideoConsoles[0].cursor;
}

void SLVideoConsoleMoveBackward(OSCount spaces)
{
    //
}

void SLVideoConsoleDelete(OSCount characters)
{
    //
}

void __SLVideoConsoleInitAll(void)
{
    SLGraphicsOutput **screens = SLGraphicsOutputGetAll();
    SLGraphicsOutput **outputs = screens;
    OSCount count = 0;

    if (!screens)
    {
        SLPrintString("Aww, no screens :'(\n");
        return;
    }

    while (*outputs)
    {
        //SLGraphicsOutput *output = (*outputs++);
        outputs++;
        count++;

        /*SLPrintString("Graphics Device [%p]: {\n", output);
        SLPrintString("    Framebuffer Base: %p\n", output->mode->framebuffer);
        SLPrintString("    Framebuffer Size: 0x%zX\n", output->mode->framebufferSize);
        SLPrintString("    Current Mode: %u of %u total modes [%p]\n\n", output->mode->currentMode, output->mode->numberOfModes);
        SLPrintString("    Modes:");

        for (OSCount i = 0; i < output->mode->numberOfModes; i++)
        {
            SLGraphicsModeInfo *modeInfo = SLGraphicsOutputGetMode(output, (UInt32)i);
            char *modeName;

            switch (modeInfo->format)
            {
                case kSLGraphicsPixelFormatRGBX8:   modeName = "RGBX_8";  break;
                case kSLGraphicsPixelFormatBGRX8:   modeName = "BGRX_8";  break;
                case kSLGraphicsPixelFormatBitMask: modeName = "BitMask"; break;
                case kSLGraphicsPixelFormatBLT:     modeName = "BLT";     break;
            }

            SLPrintString(" {\n");
            SLPrintString("        %zu: %ux%u\n", (i + 1), modeInfo->width, modeInfo->height);
            SLPrintString("        Format: %s\n", modeName);
            SLPrintString("        Pixels per Scanline: %zu\n", modeInfo->pixelsPerScanline);
            SLPrintString("    }");
        }

        SLPrintString("\n} ");*/
    }

    //SLPrintString("\n\n");
    SLGraphicsContext *context = SLGraphicsOutputGetContextWithMaxSize(screens[0], 900, 1440);
    CXKMemorySetValue(context->framebuffer, context->framebufferSize, 0x00);
    /*SLPrintString("Selected Context:\n");
    SLPrintString("{\n");
    SLPrintString("    %ux%u\n", context->width, context->height);
    SLPrintString("    %u from %p\n", context->framebufferSize, context->framebuffer);
    SLPrintString("    %zu %s Pixels\n", context->pixelCount, (context->isBGRX ? "BGRX" : "RGBX"));
    SLPrintString("}\n\n");*/

    gSLVideoConsoles[0].selectedFont = &gSLBitmapFont8x16;
    gSLVideoConsoles[0].context = context;
    gSLVideoConsoles[0].cursor = ((SLConsoleLocation){0, 0});
    gSLVideoConsoles[0].backgroundColor = 0x00000000;
    gSLVideoConsoles[0].color = 0x00FFFFFF;

    gSLVideoConsoles[0].rootConsole.id = 0x00;
    gSLVideoConsoles[0].rootConsole.output = SLVideoConsoleOutput;
    gSLVideoConsoles[0].rootConsole.input = SLVideoConsoleInput;
    gSLVideoConsoles[0].rootConsole.height = context->height / gSLVideoConsoles[0].selectedFont->height;
    gSLVideoConsoles[0].baseOffsetY = (context->height % gSLVideoConsoles[0].selectedFont->height) / 2;
    gSLVideoConsoles[0].rootConsole.width = context->width / gSLVideoConsoles[0].selectedFont->width;
    gSLVideoConsoles[0].baseOffsetX = (context->width % gSLVideoConsoles[0].selectedFont->width) / 2;
    gSLVideoConsoles[0].rootConsole.moveCursor = SLVideoConsoleMoveCursor;
    gSLVideoConsoles[0].rootConsole.getCursor = SLVideoConsoleGetCursor;
    gSLVideoConsoles[0].rootConsole.moveBackward = SLVideoConsoleMoveBackward;
    gSLVideoConsoles[0].rootConsole.delete = SLVideoConsoleDelete;

    //gSLOutputFunctions[1] = SLVCOC;
}

#endif /* kCXDebug */
