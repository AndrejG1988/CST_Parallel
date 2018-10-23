#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

#pragma warning(disable : 4996)

void dateiEinlesen();
//int hammingDistanz(char *str1, char *str2);
int hammingDistanz(uint64_t x, uint64_t y);
void printResult();

uint64_t* valueList;
uint32_t valuesCount;
uint32_t valueLength;

uint64_t bestValue;
uint64_t bestDif = -1;

int main(int argc, char **argv) {

	dateiEinlesen();

	uint64_t currDif = 0;
	uint64_t localDif = 0;

	uint64_t maxValue = pow(2,4*valueLength)-1 ;
	//printf("%llx\n", maxValue);

	// Schleife zählt von 0 bis 0xFFFFFF hoch
	for (uint64_t i = 0; i <= maxValue; i++) {
		localDif = 0;
		for (int o = 0; o < valuesCount; o++){
			// berechne unterschide
			currDif = hammingDistanz(i, valueList[o]);
			//printf("%d %06llx <> %06llx - %llu \n", o, i, valueList[o], currDif);

			if (currDif > localDif)
				localDif = currDif;

			/*
			if (currDif >= bestDif)
				break;
			*/
			
			if (currDif > localDif)
				localDif = currDif;
		}
		//printf("%06llx %llu\n", i, localDif );
		if (localDif < bestDif) {
			bestDif = localDif;
			bestValue = i;
			printf("new best: %06llx score: %llu\n", bestValue, bestDif );
		}
		//printf("%06llx\n", i);
		//printf("%llu %llu %llu\n",  bestDif, localDif, currDif);
		//exit(0);
	}

	printResult();
	system("pause");
	return 0;
}

// einlesen der Informationen aus der Quelle "strings.txt" Datei
//inhaltQuelle dateiLesen() {
void dateiEinlesen() {

	FILE *quelle;

	/* Bitte Pfad und Dateinamen anpassen */
	quelle = fopen("strings.txt", "r");
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
		printf("Laenge: %d\n", valuesCount);
	}
	printf("Inhalt der Quelle ausser die Ersten Zwei Zeilen: \n");
	//
	int count = 0;
	printf("%8s <<\tint\n", "hex");

	while (fgets(zeile, laenge, quelle) != NULL) {
		// wandle hex-string zu uint um und schreibe in liste
		valueList[count] = (uint64_t)strtol(zeile, NULL, 16);

		printf("0x%06llx <<\t%llu\n", valueList[count], valueList[count]);

		count++;
		if (count >= valuesCount)
			break;
	}
	printf("\n");

	free(zeile);
}

/*
int hammingDistanz(char *str1, char *str2)
{
	uint64_t i = 0, count = 0;
	while (str1[i] != NULL)
	{
		if (str1[i] != str2[i])
		{
			count++;
		}
		i++;
	}
	//printf("Die Hamming Distanz ist: %d \n", count);
	return count;
}
*/

int hammingDistanz(uint64_t x, uint64_t y){
	uint64_t i = 0xf;
	int count = 0;
	for (int o = 0; o <= valuesCount; o++){
		if (  __builtin_popcountll((i&x) ^ (i&y)) != 0 ){
			//printf("%llx, %llx\n", i&x, i&y);
			count++;
		}
		i <<= 4;
	}
	return count;
}


void printResult(){
	printf("======================================\n");
	printf("Closest string:\n");
	printf("Distance %llu\n", bestDif);
	printf("New string %05llx\n", bestValue );
	printf("String\tDistance\n");
	uint64_t dif;
	for (int i = 0; i < valuesCount; i++){
		dif = hammingDistanz(bestValue, valueList[i]);
		printf("%05llx\t%llu\n", valueList[i], dif);
	}
	printf("======================================\n");
}