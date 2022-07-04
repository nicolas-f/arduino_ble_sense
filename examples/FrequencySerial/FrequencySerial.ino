/*
  Circuit:
  - Arduino Nano 33 BLE board
  
  Output the frequency on the PDM microphone using the Zero Crossing algorithm
  
  This example code is in the public domain.
*/

#include <PDM.h>

// default number of output channels
static const char channels = 1;

// default PCM output frequency
#define SAMPLE_RATE 16000

// Buffer to read samples into, each sample is 16-bits
short sampleBuffer[512];



// Number of audio samples read
volatile int samplesRead;

long lastPrint = millis();

void setup() {
  
  digitalWrite(LED_PWR, LOW);
  
  
  Serial.begin(9600);
  while (!Serial);

  // Configure the data receive callback
  PDM.onReceive(onPDMdata);

  // Initialize PDM with:
  // - one channel (mono mode)
  // - a 16 kHz sample rate for the Arduino Nano 33 BLE Sense
  if (!PDM.begin(channels, SAMPLE_RATE)) {
    Serial.println("Failed to start PDM!");
    while (1);
  }
}

void loop() {
  // Wait for samples to be read
  if (samplesRead) {
    double zeroCrossSample = 0;
    double zeroCrossDelay = 0;

    // Print samples to the serial monitor or plotter
    for (int i = 1; i < samplesRead; i++) {      
      const short previous = sampleBuffer[i-1];
      const short current = sampleBuffer[i];
      if (previous >= 0 && current < 0) {
          // evaluate Tzc
          double oldZeroCross = zeroCrossSample;
          zeroCrossSample = (i - 1) + ((double)previous / (previous - current));
          if(oldZeroCross > 0) {
            double newZeroCrossDelay = zeroCrossSample - oldZeroCross;
            if(zeroCrossDelay < 1) {
              zeroCrossDelay = newZeroCrossDelay;
            }
            long now = millis();
            if(now - lastPrint > 500) {
              // Print measured frequency in Hz
              Serial.println((int)(SAMPLE_RATE / ((zeroCrossDelay + newZeroCrossDelay) / 2.0)));
              lastPrint = now;
            }
            zeroCrossDelay = newZeroCrossDelay;
          }
      }
    }
    
    // Clear the read count
    samplesRead = 0;
  }
}

/**
 * Callback function to process the data from the PDM microphone.
 * NOTE: This callback is executed as part of an ISR.
 * Therefore using `Serial` to print messages inside this function isn't supported.
 * */
void onPDMdata() {
  // Query the number of available bytes
  int bytesAvailable = PDM.available();

  // Read into the sample buffer
  PDM.read(sampleBuffer, bytesAvailable);

  // 16-bit, 2 bytes per sample
  samplesRead = bytesAvailable / 2;
}
