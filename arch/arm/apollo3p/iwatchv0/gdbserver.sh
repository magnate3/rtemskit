#!/bin/sh

JLinkGDBServerExe -select USB -device AMA3B2KK-KBR -endian little -if SWD -speed 2000 -ir -noLocalhostOnly &