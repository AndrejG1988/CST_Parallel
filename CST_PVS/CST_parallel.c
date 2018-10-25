#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <unistd.h>

#ifdef _MSC_VER
#  include <intrin.h>
#  define __builtin_popcountll __popcnt64
#endif

#pragma warning(disable : 4996)

//#define WithScore // langsamer 

int verbos = 3;
char* file = NULL;

void dateiEinlesen();
//int hammingDistanz(char *str1, char *str2);
int hammingDistanz(uint64_t x, uint64_t y);
void printResult();
void run();

uint64_t* valueList; // speicher alle eingelesenen Werte als unsigned long long (max 16 Zeichen)
uint32_t valuesCount;
uint32_t valueLength;

uint64_t bestValue;
uint64_t bestDif = -1;
uint64_t bestScore = -1;

int main(int argc, char **argv) {
	int opt = 0;
	int temp = 0;

	while ((opt = getopt(argc, argv, "v:f:")) != -1) {
		switch(opt){
		case 'v':
		temp = atoi(optarg);
		if (temp >= 0 && temp <= 3){
			printf("Verbos: %d\n", temp);
			verbos = temp;	
		}
		break;
		case 'f':
		file = optarg;
		printf("input file: %s\n", file);
		break;
		case '?':
		printf("wtf\n");
		break;
		default:
		printf("hmmmm\n");

		}
	}

	if (file == NULL){
		file = "strings.txt";
	}

	dateiEinlesen();
	clock_t begin = clock();

	run();

	clock_t end = clock();
	double time_spent = (double)(end - begin) / CLOCKS_PER_SEC;

	printf("duration: %f\n", time_spent);
	printResult();
#ifdef _MSC_VER
	system("pause");
#endif
	return 0;
}

void run(){
	uint64_t currDif = 0;
	uint64_t localDif = 0;
#ifdef WithScore
	uint64_t sum;
#endif

	uint64_t maxValue = pow(2,4*valueLength)-1 ;
	//printf("%llx\n", maxValue);

	// Schleife zählt von 0 bis 0xFFFFFF hoch (in newValue)
	for (uint64_t newValue = 0; newValue <= maxValue; newValue++) { 
		localDif = 0;
#ifdef WithScore
		sum = 0;
#endif
		for (int i = 0; i < valuesCount; i++){
			// berechne unterschide
			currDif = hammingDistanz(newValue, valueList[i]);
#ifdef WithScore
			// summiere alle Unterschidswerte
			sum += currDif;
#endif

			// speichert höchste differenz zu newValue
			if (currDif > localDif)
				localDif = currDif;

#ifdef WithScore
			// beginnt mit nächstem iteration für newValue bei:
			if (sum >= bestScore || 
				currDif > bestDif)
				break;
#else
			if (currDif >= bestDif)
				break;
#endif


		}

#ifdef WithScore
		if (localDif < bestDif && sum < bestScore ) {
			bestScore = sum;
			bestDif = localDif;
			bestValue = newValue;
			//printf("new best: %0*llx dif: %llu sum: %llu\n", (int)valueLength, bestValue, bestDif, bestScore );
		}
#else
		if (localDif < bestDif ) {	
			bestDif = localDif;
			bestValue = newValue;
			//printf("new best: %0*llx dif: %llu\n", (int)valueLength, bestValue, bestDif );
		}
#endif

	}
}

// einlesen der Informationen aus der Quelle "strings.txt" Datei
//inhaltQuelle dateiLesen() {
void dateiEinlesen() {

	FILE *quelle;

	/* Bitte Pfad und Dateinamen anpassen */
	quelle = fopen(file, "r");
	size_t laenge = 255;

	// Speicher reservieren
	char* zeile = (char*)malloc(sizeof(char)*laenge);

	//prüfe ob die datei auch existiert
	if (NULL == quelle) {
		printf("Konnte Datei \"test.txt\" nicht oeffnen!\n");
		return;
	}

	//erste Zeile der Text datei auslesen und in eine Variabele speichern, mit fgets(ziel, n menge, quelle).
	if (fgets(zeile, laenge, quelle) != NULL) {

		valuesCount = atoi(zeile);
		printf("Groesse: %d\n", valuesCount);
		valueList = (uint64_t*)malloc(valuesCount * sizeof(uint64_t));
	}

	//zweite Zeile der Text datei auslesen und in eine Variabele speichern, mit fgets(ziel, n menge, quelle).
	if (fgets(zeile, laenge, quelle) != NULL) {

		valueLength = atoi(zeile);
		printf("Laenge: %d\n", valueLength);
		if (valueLength > 16){
			printf("Zeichenkette zu lang(max 16). Dateinvormat kann nicht allles speichern\n");
			exit(2);
		}
	}
	printf("Inhalt der Quelle ausser die Ersten Zwei Zeilen: \n");
	//
	int count = 0;
	printf(" i. hex\n");

	while (fgets(zeile, laenge, quelle) != NULL) {
		// wandle hex-string zu uint um und schreibe in liste
		valueList[count] = (uint64_t)strtol(zeile, NULL, 16);

		printf("%2d. 0x%0*llx \n", count, (int)valueLength, valueList[count]);

		count++;
		if (count >= valuesCount)
			break;
	}
	printf("\n");

	free(zeile);
}

int hammingDistanz(uint64_t x, uint64_t y){
	uint64_t i = 0xf;
	int count = 0;
	for (int o = 0; o <= valuesCount; o++){
		if (  __builtin_popcountll((i&x) ^ (i&y)) != 0 ){
			count++;
		}
		i <<= 4;
	}
	return count;
}


void printResult(){
	int vSize = (int)valueLength;
	printf("======================================\n");
	printf("Closest string:\n");
	printf("Distance %llu\n", bestDif);
	printf("New string %0*llx\n", vSize, bestValue );
	printf("%*s\tDistance\n", vSize, "String");
	uint64_t dif;
	for (int i = 0; i < valuesCount; i++){
		dif = hammingDistanz(bestValue, valueList[i]);
		printf("%0*llx\t%llu\n", vSize, valueList[i], dif);
	}
#ifdef WithScore
	printf("--------------------------------------\n");
	printf("%*s\t%llu\n", vSize, "sum:", bestScore);
#endif
	printf("======================================\n");
}