#ifndef WEATHER_H
#define WEATHER_H

typedef struct weather { int temperature; int humidity; } Weather; // is this right

int get_api_key(char*);

int write_api_key(char*);

//size_t curl_callback(char*, size_t, size_t, void*);

//void set_zip(Weather* w);

void weather_initialize(Weather*);

#endif
