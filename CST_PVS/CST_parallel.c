#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <mpi.h>
#include <stdint.h>
#include <string.h>
/*
	Um MPI ausführen zu können muss man mpiexec -n 4 Clostest_String_Problem.exe -s1 -v3 -f strings16.txt eingeben.
*/


#ifdef _MSC_VER
#   include "getopt.h" // extra library für argument übergabe
#	include <intrin.h>
//#	define __builtin_popcountll __popcnt64
#else 
#	include <unistd.h>
#   define __popcnt64 __builtin_popcountll
#endif

#define DEBUG

#pragma warning(disable : 4996)

int verbos = 3;
char* file = "strings.txt";

void initArguments(int argc, char **argv);
void dateiEinlesen();

void run(uint64_t start, uint64_t stop);
int hammingDistanz(uint64_t x, uint64_t y);

void printResult();


// MPI Variablen
int rank, numprocs;

typedef struct Input {
	uint64_t* values;
	uint32_t count;
	uint32_t length;
} Input;
Input input = {.count = -1};

// eingelesene Daten
//uint64_t* valueList; // speicher alle eingelesenen Werte als unsigned long long (max 16 Zeichen)
//uint32_t valuesCount = -1;
//uint32_t valueLength;

typedef struct Result {
	uint64_t value;
	uint64_t differenz;
} Result;
Result best = { .differenz = -1};
// bestes Ergebnis
//uint64_t bestValue;
//uint64_t bestDif = -1;

uint64_t* allBestValues;
uint64_t* allBestDif;

int main(int argc, char **argv) {

	// MPI Init
	int node;
	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &node);

	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &numprocs);
#ifdef DEBUG
	if (node == 0)
		printf("num Nodes: %d\n", numprocs);
	printf("Hello World from Node %d\n", node);
#endif

	initArguments(argc, argv);

	dateiEinlesen();

	/*variables used for gathering timing statistics*/
	double mytime,
		maxtime,
		mintime,
		avgtime;

	uint64_t start, stop;

	MPI_Barrier(MPI_COMM_WORLD);  /*synchronize all processes*/
	clock_t begin = clock();
	mytime = MPI_Wtime();  /*get time just before work section */

	uint64_t maxValue = pow(2, 4 * input.length);

	start = (uint64_t)((maxValue * rank) / numprocs);
	stop = (uint64_t)((maxValue * (rank + 1)) / numprocs);

	run(start, stop);

	// sync Values, Dif 
	if (rank == 0) {
		allBestValues = (uint64_t*)malloc(numprocs * sizeof(uint64_t));
		MPI_Gather(
			&(best.value), 			// void* send_data, 
			1, 						// int send_count,
			MPI_UNSIGNED_LONG_LONG, // MPI_Datatype send_datatype,
			allBestValues, 			// void* recv_data,
			1, 						// int recv_count,
			MPI_UNSIGNED_LONG_LONG,	// MPI_Datatype recv_datatype,
			0,						// int root,
			MPI_COMM_WORLD);		// MPI_Comm communicator)
		allBestDif = (uint64_t*)malloc(numprocs * sizeof(uint64_t));
		MPI_Gather(
			&(best.differenz), 			// void* send_data, 
			1, 						// int send_count,
			MPI_UNSIGNED_LONG_LONG, // MPI_Datatype send_datatype,
			allBestDif, 			// void* recv_data,
			1, 						// int recv_count,
			MPI_UNSIGNED_LONG_LONG,	// MPI_Datatype recv_datatype,
			0,						// int root,
			MPI_COMM_WORLD);		// MPI_Comm communicator)
	}
	else {
		MPI_Gather(&(best.value), 1, MPI_UNSIGNED_LONG_LONG, NULL, 0, MPI_UNSIGNED_LONG_LONG, 0, MPI_COMM_WORLD);
		MPI_Gather(&(best.differenz), 1, MPI_UNSIGNED_LONG_LONG, NULL, 0, MPI_UNSIGNED_LONG_LONG, 0, MPI_COMM_WORLD);
	}


	// bestes Ergebnis raussuchen
	if (rank == 0) {
		for (int p = 0; p < numprocs; p++) {
			if (best.differenz > allBestDif[p]) {
				best.differenz = allBestDif[p];
				best.value = allBestValues[p];
			}
		}
	}

	clock_t end = clock();
	mytime = MPI_Wtime() - mytime;  /*get time just after work section*/
	double time_spent = (double)(end - begin) / CLOCKS_PER_SEC;

	/*compute max, min, and average timing statistics*/
	MPI_Reduce(&mytime, &maxtime, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
	MPI_Reduce(&mytime, &mintime, 1, MPI_DOUBLE, MPI_MIN, 0, MPI_COMM_WORLD);
	MPI_Reduce(&mytime, &avgtime, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);

	if (rank == 0) {
		if (verbos == 1 || verbos == 3)
			printResult();
		if (verbos >= 2) {
			avgtime /= numprocs;
			printf("MPI time messure: Min: %lf  Max: %lf  Avg:  %lf\n", mintime, maxtime, avgtime);
			printf("clock duration: %f sec\n", time_spent);
		}
	}

	MPI_Finalize();
#ifdef _MSC_VER
	system("pause");
#endif
	return 0;
}

void initArguments(int argc, char **argv) {
	// optional arguments init
	int opt = 0;
	int temp = 0;
	while ((opt = getopt(argc, argv, "v:f:s:")) != -1) {
		switch (opt) {
		case 'v':
			temp = atoi(optarg);
			if (temp >= 0 && temp <= 3) {
				//printf("Verbos: %d\n", temp);
				verbos = temp;
#ifdef DEBUG
				if (rank == 0) printf("Verbos set to %d\n", verbos);
#endif
			}
			break;
		case 'f':
			file = optarg;
#ifdef DEBUG
			if (rank == 0) printf("input file: %s\n", file);
#endif
			break;
		case 's':
			input.count = (uint32_t)atoi(optarg);
#ifdef DEBUG
			if (rank == 0) printf("max num of Strings set to: %u\n", input.count);
#endif
			break;
		case '?':
			printf("wtf\n");
			break;
		default:
			printf("hmmmm\n");
		}
	}
}

void run(uint64_t start, uint64_t stop) {
	uint64_t currDif = 0;
	uint64_t localDif = 0;


	// Schleife zählt von 0 bis 0xFFFFFF hoch (in newValue)
	for (uint64_t newValue = start; newValue < stop; newValue++) {
		localDif = 0;

		for (int i = 0; i < input.count; i++) {
			// berechne unterschide
			currDif = hammingDistanz(newValue, input.values[i]);

			// speichert höchste differenz zu newValue
			if (currDif > localDif)
				localDif = currDif;

			if (currDif >= best.differenz)
				break;
		}

		if (localDif < best.differenz) {
			best.differenz = localDif;
			best.value = newValue;
			//printf("new best: %0*llx dif: %llu\n", (int)valueLength, bestValue, best.differenz );
		}
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
	if (quelle == NULL) {
		printf("Konnte Datei \"%s\" nicht oeffnen!\n", file);
		exit(2);
	}

	//erste Zeile der Text datei auslesen und in eine Variabele speichern, mit fgets(ziel, n menge, quelle).
	if (fgets(zeile, laenge, quelle) != NULL) {

		if (input.count > (uint32_t)atoi(zeile))
			input.count = (uint32_t)atoi(zeile);

#ifdef DEBUG
		if (rank == 0) printf("input.count: %d\n", input.count);
#endif
		input.values = (uint64_t*)malloc(input.count * sizeof(uint64_t));
	}

	//zweite Zeile der Text datei auslesen und in eine Variabele speichern, mit fgets(ziel, n menge, quelle).
	if (fgets(zeile, laenge, quelle) != NULL) {

		input.length = atoi(zeile);
#ifdef DEBUG
		if (rank == 0) printf("input.length: %d\n", input.length);
#endif
		if (input.length > 16) { // evetl wieder auf 15 wechseln
			printf("Zeichenkette zu lang(max 15). Dateinvormat kann nicht allles speichern\n");
			exit(3);
		}
	}


#ifdef DEBUG
	if (rank == 0) printf("Inhalt der Quelle ausser die Ersten Zwei Zeilen: \n i. hex\n");
#endif
	int count = 0;
	while (fgets(zeile, laenge, quelle) != NULL) {
		// wandle hex-string zu uint um und schreibe in liste
		input.values[count] = (uint64_t)strtoll(zeile, NULL, 16);

#ifdef DEBUG
		if (rank == 0) printf("%2d. 0x%0*llx \n", count, (int)input.length, input.values[count]);
#endif

		count++;
		if (count >= input.count)
			break;
	}
#ifdef DEBUG
	if (rank == 0) printf("\n");
#endif

	free(zeile);
}

int hammingDistanz(uint64_t x, uint64_t y) {
	uint64_t i = 0xf;
	int count = 0;
	for (int o = 0; o <= input.length; o++) {
		if (__popcnt64((i&x) ^ (i&y)) != 0) {
			count++;
		}
		i <<= 4;
	}
	return count;
}


void printResult() {
	int vSize = (int)input.length;
	printf("======================================\n");
	printf("Closest string:\n");
	printf("Distance %llu\n", best.differenz);
	printf("New string %0*llx\n", vSize, best.value);
	printf("%*s\tDistance\n", vSize, "String");
	uint64_t dif;
	for (int i = 0; i < input.count; i++) {
		dif = hammingDistanz(best.value, input.values[i]);
		printf("%0*llx\t%llu\n", vSize, input.values[i], dif);
	}
	printf("======================================\n");
}