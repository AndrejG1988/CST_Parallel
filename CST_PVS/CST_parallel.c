#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#pragma warning(disable : 4996)

void dateiEinlesen();
uint64_t hammingDistanz(char *str1, char *str2);


int main(int argc, char **argv) {

	//dateiEinlesen();
	//Testvariables
	char str1[] = "123456";
	char str2[] = "123654";


	char buf[] = "000000";
	uint64_t dif = 0;
	uint64_t bestDif = 0-1;
	//printf("%llu\n", bestDif);
	//exit(0);
	// Schleife zählt von 0 bis 0xFF hoch
	for (uint64_t i = 0; i <= 0xFFFFFF; i++) {
		//convert uint in hex String
		sprintf(buf, "%06llx", i);
		//printf("%s <> %s = ", buf, str1);
		
		// berechne unterschide
		dif = hammingDistanz(str1, buf);
		//printf("%d\n", dif);

		if (dif < bestDif){
			// set new best score
			bestDif = dif;
			printf("%s <> %s = %llu\n", str1, buf, dif );
		}
	}
	exit(0);
	hammingDistanz(str1, str2);
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

	int stringGroesse = 0;
	int stringLaenge = 0;
	char** inhaltQuelle;
	// Speicher reservieren
	char* zeile = (char*)malloc(sizeof(char)*laenge);

	//prüfe ob die datei auch existiert
	if (NULL == quelle) {
		printf("Konnte Datei \"test.txt\" nicht oeffnen!\n");
		return;
	}

	//erste Zeile der Text datei auslesen und in eine Variabele speichern, mit fgets(ziel, n menge, quelle).
	if (fgets(zeile, laenge, quelle) != NULL) {

		stringGroesse = atoi(zeile);
		printf("Groesse: %s\n", zeile);
	}

	//zweite Zeile der Text datei auslesen und in eine Variabele speichern, mit fgets(ziel, n menge, quelle).
	if (fgets(zeile, laenge, quelle) != NULL) {
		stringLaenge = atoi(zeile);
		printf("Laenge: %s\n", zeile);
	}
	printf("Inhalt der Quelle ausser die Ersten Zwei Zeilen: \n\n");
	//
	while (fgets(zeile, laenge, quelle) != NULL) {
		int count = 0;
		//printf("%d\n", stringGroesse);
		inhaltQuelle = (char**)malloc(stringGroesse * sizeof * inhaltQuelle);
		inhaltQuelle[count] = zeile;
		//printf("%d\n", sizeof(void*));
		printf("%s\n", inhaltQuelle[count]);
		count++;
	}
	printf("\n");

	free(zeile);
}

uint64_t hammingDistanz(char *str1, char *str2)
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

