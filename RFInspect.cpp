/*
RFInspect - analyze mono WAV files for line code protocol

Usage: ./RFInspect <WAV file> [<pulseLength>] [-v]
	[] = optional

Adapted from modified RFSniffer.cpp:
  https://github.com/Martin-Laclaustra/433Utils/blob/verbosesnifferwithprotocollessreceiver/RPi_utils/RFSniffer.cpp

by @justy to provide a handy RF code sniffer
by @Martin-Laclaustra to provide a protocol generator
by @OrbitTheSun to accept single channel (mono) WAV files as input
*/

#include "RCSwitch.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <ctype.h>
// required to compute squareroot to get a standard deviation of timings
#include <math.h>

RCSwitch mySwitch;


int main(int argc, char *argv[])
{
        int c, verbose = 0;
        opterr = 0;

        while ((c = getopt(argc, argv, "v")) != -1)
                switch (c)
                {
                case 'v':
                    verbose++;
                    break;
                case '?':
                    if (isprint(optopt))
                        fprintf(stderr, "Unknown option `-%c'.\n", optopt);
                    else
                        fprintf(stderr, "Unknown option character `\\x%x'.\n", optopt);
                    return 1;
                default:
                    abort();
                }

        if (optind >= argc)
        {
                fprintf(stderr, "Usage: RFInspect <mono WAV file> [<pulseLength>] [-v]\n");
                return 0;
        }

	int pulseLength = 0;
        if (optind + 1 < argc) pulseLength = atoi(argv[optind + 1]);


	mySwitch = RCSwitch();
	if (pulseLength != 0) mySwitch.setPulseLength(pulseLength);

        mySwitch.processWAV(argv[optind]);

        int value = mySwitch.getReceivedValue();

        if (value == 0) {
                printf("Unknown encoding\n");
        } else {

                printf("Received %i\n", mySwitch.getReceivedValue() );

                if ((argc > 1 && strcmp(argv[1],"-v") == 0) || (argc > 2 && strcmp(argv[2],"-v") == 0)) {
                        int nbitlength = mySwitch.getReceivedBitlength();
                        int numberoftimings = 2 * nbitlength + 2;
                        int databuffer[64]; // get a copy of the received timings before they are overwritten
                        if(numberoftimings > 64) numberoftimings = 64;
                        for (int i = 0; i < numberoftimings; i++) {
                                databuffer[i] = mySwitch.getReceivedRawdata()[i];
                        }

                        int nreceived = mySwitch.getReceivedValue();
                        int nprotocol = mySwitch.getReceivedProtocol();
                        int npulselength = mySwitch.getReceivedDelay();

                        printf("\n=============================================\n");
                        printf("=============================================\n");
                        printf("\n");
                        printf("GENERAL\n");
                        printf("=======\n");
                        printf("Received %i\n", nreceived );
                        printf("Bitlength %i\n", nbitlength );
                        printf("Protocol %i\n", nprotocol );
                        printf("Delay(pulse length in us) %i\n", npulselength );
                        printf("(delay provided from rc-switch)\n");

                        printf("\n");
                        printf("STATISTICS\n");
                        printf("==========\n");

                        unsigned int databitsoffset = abs( (int)mySwitch.getReceivedLevelInFirstTiming() - (int)mySwitch.getReceivedInverted());

                        // compute duration of data bits
                        unsigned long dataduration = 0;
                        for (unsigned int i = 1 + databitsoffset; i < numberoftimings - 1 + databitsoffset; i++) {
                                dataduration += databuffer[i];
                        }
                        printf("Duration of data bits in pulse train :  %i\n", dataduration );

                        // compute average databit duration
                        unsigned int averagebitduration = 0;
                        if(nbitlength != 0) {
                                averagebitduration = (int) (0.5 + (double)dataduration/(double)nbitlength); // warning, ensure avoiding 0 division (should not happen because rc-switch rejects < 4 bits)
                        }
                        // compute databit duration variability
                        int variancebitduration = 0;
                        for (unsigned int i = 1 + databitsoffset; i < numberoftimings - 1 + databitsoffset; i += 2) {
                                variancebitduration += (databuffer[i]+databuffer[i + 1]-averagebitduration)*(databuffer[i]+databuffer[i + 1]-averagebitduration);
                        }
                        if((nbitlength-1) != 0) {
                                variancebitduration /= (nbitlength-1); // warning, ensure avoiding 0 division (should not happen because rc-switch rejects < 4 bits)
                        } else {
                                variancebitduration = 0;
                        }
                        double sdbitduration = sqrt(variancebitduration);

                        printf("Data bit duration = %i and standard deviation = %.2f\n", averagebitduration, sdbitduration );

                        double coefficientofvariation = sdbitduration / averagebitduration;
                        printf("Coefficient of variation of data-bit duration = %.2f % (should be less than 10%)\n", 100*coefficientofvariation );
                        printf("Do not use the rest of the information if big coefficient of variation\n");
                        unsigned int protocolratio = (unsigned int)(0.5 + (double)(averagebitduration-npulselength)/(double)npulselength);
                        printf("Long-to-short duration ratio for data bits (rounded) = %i\n", protocolratio );

                        // sync bit
                        if(databitsoffset == 0) {
                                printf("Sync bit (in multiples of the pulseLength) = %i %i\n", (int) (0.5 + (double)databuffer[2*nbitlength+1]/(double)npulselength) ,(int) (0.5 + (double)databuffer[0]/(double)npulselength) );
                        } else {
                                printf("Sync bit (in multiples of the pulseLength) = %i %i\n", (int) (0.5 + (double)databuffer[0]/(double)npulselength) ,(int) (0.5 + (double)databuffer[1]/(double)npulselength) );
                        }
                        printf("\n");
                        printf("Proposed protocol for RCswitch\n");
                        printf("{ %i, { %i, %i }, { 1, %i }, { %i, 1 }, %s }\n",
                        npulselength,
                        (databitsoffset==0) ?
                                (int) (0.5 + (double)databuffer[2*nbitlength+1]/(double)npulselength)
                                :
                                (int) (0.5 + (double)databuffer[0]/(double)npulselength),
                        (databitsoffset==0) ?
                                (int) (0.5 + (double)databuffer[0]/(double)npulselength)
                                :
                                (int) (0.5 + (double)databuffer[1]/(double)npulselength),
                        protocolratio,
                        protocolratio,
                        (mySwitch.getReceivedInverted()) ? "true" : "false");

                        printf("\n");
                        printf("STATISTICS OF VARIATION BY LEVELS\n");
                        printf("=================================\n");
                        printf("(Differences here are probably artifacts from signal creation or detection)\n");
                        printf("(This might be completely ignored, but pay attention to them if big differences are present and emission does not work)\n");

                        int longup = 0;
                        int shortup = 0;
                        int longdown = 0;
                        int shortdown = 0;
                        int longupdownshortupdown[4] = {0,0,0,0}; // values

                        // high level long and short timings
                        for (unsigned int i = 1 + databitsoffset; i < 2*nbitlength + databitsoffset; i += 2) {
                                int firstisshortlevel = (int)(databuffer[i]<databuffer[i + 1]);
                                //int firstisdown = (int)(mySwitch.getReceivedInverted());
                                longupdownshortupdown[2*firstisshortlevel+(int)(mySwitch.getReceivedInverted())] += databuffer[i];
                                longupdownshortupdown[2*(1-firstisshortlevel)+(1-(int)(mySwitch.getReceivedInverted()))] += databuffer[i + 1];
                        }
                        longup = longupdownshortupdown[0];
                        shortup = longupdownshortupdown[2];
                        longdown = longupdownshortupdown[1];
                        shortdown = longupdownshortupdown[3];
                        // low level long and short timings
                        if(nreceived != 0) {
                                int numberofsetbits = __builtin_popcount(nreceived);
                                printf("number of bits set (in %i): %i\n",nreceived,numberofsetbits);
                                if(numberofsetbits == 0 || numberofsetbits == nbitlength) { // ensure avoiding 0 division
                                        printf("all bits of the same value, skipping fractional analysis.");
                                } else {
                                        if(mySwitch.getReceivedInverted()) {
                                        longup /= nbitlength-numberofsetbits;
                                        shortdown /= nbitlength-numberofsetbits;
                                        shortup /= numberofsetbits;
                                        longdown /= numberofsetbits;
                                        } else {
                                        longup /= numberofsetbits;
                                        shortdown /= numberofsetbits;
                                        shortup /= nbitlength-numberofsetbits;
                                        longdown /= nbitlength-numberofsetbits;
                                        }
                                        printf("longup, longdown, shortup, shortdown\n");
                                        printf("%i, %i, %i, %i\n",longup, longdown, shortup, shortdown);
                                        printf("this might be useful for tweaking emmitting algorithms\n");
                                }
                        }
                        printf("\n=============================================\n");
                        // raw signal
                        if(2*nbitlength+2 < 64) { //prevent bufferoverrun
                                printf("\n");
                                printf("RAW SIGNAL (us)\n");
                                printf("===============\n");
                                printf("first level %s \n", (mySwitch.getReceivedLevelInFirstTiming() == 0) ? "down" : "up" );
                                for (int i = 0; i < 2*nbitlength + 2 - 1 + databitsoffset; i++) {
                                        printf("%i ", databuffer[i] );
                                        if((i - databitsoffset) % 16 == 0) printf("\n");
                                }
                                if ((2*nbitlength+2 - 1 + databitsoffset - 1) % 16 != 0) printf("\n");
                                if (databitsoffset != 1) printf("%i ", databuffer[2*nbitlength+1]);

                                printf("\n");
                                printf("=============================================\n");
                                printf("\n");
                                printf("\n");
                                printf("\n");
                        }
                }
        }
}
