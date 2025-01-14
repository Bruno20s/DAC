#ifndef LCDMANAGER_H
#define LCDMANAGER_H

#include <LiquidCrystal_I2C.h>

extern LiquidCrystal_I2C lcd;

void initializeLCD();
void showReport();

#endif