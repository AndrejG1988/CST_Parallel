/*
		PROGRAMM ZUM LÖSEN DES CLOSEST-STRING-PROBLEMS. MIT HILFE VON MPI.

		Author: AndrejG1988

		Wie man das Programm mit Hilfe von MPI Startet

		mpiexec -n 4 Closest_String_Problem.exe -v 3 -s 2 -f strings.txt

		Folgende Parameter sind möglich:
		*--------------------------------------------------------------------*
		| Parameter | Empfohlene Eingaben|	Bedeutung						 |
		|-----------|--------------------|-----------------------------------|
		| -n		| 1-n                |Anzahl an Prozessen                |
		|-----------|--------------------|-----------------------------------|
		| -v		| 0-3				 |Verbosity / Ausgabe / Result       |
		|-----------|--------------------|-----------------------------------|
		| -s		| 2-n	             |Anzahl der zu prüfenden Strings 	 |
		|-----------|--------------------|-----------------------------------|
		| -f		| strings.txt	     |Text-Datei zum Einlesen            |
		*--------------------------------------------------------------------*			

		Verbosity (-v):
			0 = Das Programm wird am ende keine Ausgabe anzeigen
			1 = Programm wird die Ergebnisse Ausgeben
			2 = Programm wird die Zeit Ausgeben die es für das Ausführen benötigt hat
			3 = Programm wird die Ergebnisse und die Zeit Ausgeben
*/
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <mpi.h>
#include "getopt.h"

#pragma warning(disable : 4996)

#define uint unsigned int

//	Struct Block mit eingelesene Daten
typedef struct Input {
	uint** values;			// Zahlenliste
	uint count;				// Anzahl der Zahlen (erste Zeile)
	uint base;				// Zahlensystem (zweite Zeile)
	uint length;			// Zifferlänge
} Input;

// Struct Block mit bestes Ergebnis
typedef struct Result {
	uint length;			// Zifferlänge
	uint* value;			// Beste/r Zifferkombination/String
	uint distance;			// Beste Differenz bzw. Distanz
} Result;

// Struct Block mit Status der berechnung
typedef struct State {
	uint length;			// Zifferlänge
	uint base;				// Alphabetgröße bzw. Basis
	uint* val;				// akt. Zifferkombination/Zahl
	uint* endVal;			// abbruch Zifferkombination/Zahl (wird nicht mehr berechnet)
	uint pos;				// akt. Position im Array
	uint endPos;			// Anzahl der Übereinstimmung der aktuellen Zahl und der abbruch Zahl
	int lastMove;			// letzter Schritt den gemacht wurde
} State;

void initArguments(int argc, char **argv);

void readFile(char* file);
void printResult();

uint* calcPoint(uint base, uint length, uint rank, uint numprocs);

void runSolve(uint* curValue, uint* endValue);

uint compareWithInputValues(uint* s, uint length);
uint hammingDistance(uint* v1, uint* v2, uint length);

char* uint2String(uint* integers, uint size);
uint stringNrSize(char* string, int base);
uint* string2Uint(char* string, uint base);

Result newResult(uint length, uint* value, uint distance);

State newState(uint length, uint base, uint* curVal, uint* endVal);
int stateValInc(State* s);
void stateNext(State* s);

void freeInput(Input* input);
void freeResult(Result* result);
void freeState(State* s);


//	Globale Variable
Input input = { .length = -1,
				.count = -1 };

//	Globale Variable. Erklärung: Wäre mit Pointer übergabe möglich. Doch es ist umständlich, da es in der Main eine best Vaiable gewählt wurde.
//	Die dann an runSolve übergeben werden muss. RunSolve muss dann best nochmal an newResult weitergeben. Das ist blödsinn.
Result best = { .distance = -1,
				.length = 0,
				.value = NULL };

// Globlae Vaiable falls keine Parameter gewählt wurden oder keine Text datei gewählt wurde.
int verbos = 3;
char* file = "strings7.txt";

// MPI Variablen
int rank, numprocs;

//	Main Funktion
int main(int argc, char **argv) {

	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &numprocs);

	initArguments(argc, argv);

	readFile(file);

	uint* start;
	uint* stop;

	// Variablen für MPI Zeit Statistik
	double mytime,
		maxtime,
		mintime,
		avgtime;

	uint* gatheredValues;			// Einfacher Array kein doppelter Array. Weil bei MPI_GATHER ein Einfacher Pointer übergeben werden muss.
	uint* gatheredDifferences;		// Einfacher Array 

	gatheredValues = (uint*)malloc(numprocs * input.length * sizeof(uint));		// Speicher reservieren ( Einfacher Array = anzahl Prozessoren * der Länge der Integers die übergeben wird) 
	gatheredDifferences = (uint*)malloc(numprocs * sizeof(uint));				// Speicher reservieren

	// Stoppe Zeit
	MPI_Barrier(MPI_COMM_WORLD);	// syncronisation der Prozesse
	clock_t begin = clock();
	mytime = MPI_Wtime();			// zeitmessung bevor Arbeitssektion

	// Berechne Start/End-Punkt
	start = calcPoint(input.base, input.length, rank, numprocs);
	if (rank == (numprocs - 1)) {
		stop = (uint*)calloc(input.length, sizeof(uint));
		stop[0] = input.base;
	}
	else
		stop = calcPoint(input.base, input.length, rank + 1, numprocs);

	// Suche Lösung
	runSolve(start, stop);

	mytime = MPI_Wtime() - mytime;  //gibt zeit nach der Arbeitssektion

	if (rank == 0) {
		MPI_Gather(
			best.value, 			// void* send_data/sendepuffer, 
			input.length, 			// int send_count/Nachrichtenlänge,
			MPI_UNSIGNED, 			// MPI_Datatype send_datatype/Datentyp(senden,
			gatheredValues, 		// void* recv_data/Empfangspuffer, Einfach Pointer/ doppelter nicht möglich.
			input.length, 			// int recv_count/Datentyp(Empfangen),
			MPI_UNSIGNED,			// MPI_Datatype recv_datatype/Datentyp(Empfang),
			0,						// int root,
			MPI_COMM_WORLD);		// MPI_Comm communicator

		MPI_Gather(
			&(best.distance), 		// void* send_data, 
			1, 						// int send_count,
			MPI_UNSIGNED, 			// MPI_Datatype send_datatype,
			gatheredDifferences, 	// void* recv_data,
			1, 						// int recv_count,
			MPI_UNSIGNED,			// MPI_Datatype recv_datatype,
			0,						// int root,
			MPI_COMM_WORLD);		// MPI_Comm communicator)
	}
	else {
		MPI_Gather(best.value, input.length, MPI_UNSIGNED, NULL, 0, MPI_UNSIGNED, 0, MPI_COMM_WORLD);
		MPI_Gather(&(best.distance), 1, MPI_UNSIGNED, NULL, 0, MPI_UNSIGNED, 0, MPI_COMM_WORLD);
	}

	// root(rank 0) bestes Ergebnis raussuchen
	if (rank == 0) {
		for (int p = 0; p < numprocs; p++) {
			if (best.distance > gatheredDifferences[p]) {		// Ist ergebnis besser als das Ergebenis was übermittelt wurde
				freeResult(&best);								// wenn neues ergebnis besser als altes, dann altes ergebis freimachen (löschen)
				//	Trick: Eigentlich doppelter Array. Mit einfachen Array umgangen. Übergeben der länge, übergabe was in gatherValues[rank*längeString] empfangen wurde(erste Adresse des Werte Paars)
				best = newResult(input.length, &(gatheredValues[p*input.length]), gatheredDifferences[p]);
			}
		}
	}



	clock_t end = clock();											// Stoppe Zeit
	double time_spent = (double)(end - begin) / CLOCKS_PER_SEC;		// Berechnet die Zeit in Sekunden


	//	compute max, min, and average timing statistics
	MPI_Reduce(&mytime, &maxtime, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
	MPI_Reduce(&mytime, &mintime, 1, MPI_DOUBLE, MPI_MIN, 0, MPI_COMM_WORLD);
	MPI_Reduce(&mytime, &avgtime, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);

	//	hier wird das ergebnis Ausgegeben falls verbosity nicht 0 gewählt wurde
	if (rank == 0) {
		if (verbos == 1 || verbos == 3)
			printResult();
		if (verbos >= 2) {
			avgtime /= numprocs;
			printf("MPI time messure: Min: %lf  Max: %lf  Avg:  %lf\n", mintime, maxtime, avgtime);
			printf("clock duration: %f sec\n", time_spent);
		}
	}

	// Es wurde für die Zeitmessung die Verbosity angepasst damit die Gemessenen Zeiten in eine CSV Datei geschrieben werden
	//	hier wird das ergebnis Ausgegeben falls verbosity nicht 0 gewählt wurde
	//if (rank == 0) {
	//	if (verbos == 1 || verbos == 3)
	//		printResult();
	//	if (verbos == 2 || verbos == 3) {
	//		avgtime /= numprocs;
	//		printf("MPI time messure: Min: %lf  Max: %lf  Avg:  %lf\n", mintime, maxtime, avgtime);
	//		printf("clock duration: %f sec\n", time_spent);
	//	}
	//	if (verbos == 4) { // für csv Datei
	//		printf("%d, %lf, %lf, %lf, %lf, %s\n", numprocs, maxtime, mintime, avgtime, time_spent, file);
	//	}
	//}

	freeInput(&input);
	freeResult(&best);
	MPI_Finalize();
#ifdef _MSC_VER
	system("pause");
#endif
	return 0;
}

//	initArguments ist für die Parameter beim der Porgramm ausführung zuständig
void initArguments(int argc, char **argv) {
	// optional arguments init
	int opt = 0;
	int temp = 0;
	while ((opt = getopt(argc, argv, "v:f:s:")) != -1) {
		switch (opt) {
		case 'v':
			temp = atoi(optarg);
			if (temp >= 0 && temp <= 3) {
				verbos = temp;
			}
			break;
		case 'f':
			file = optarg;

			break;
		case 's':
			input.count = (uint)atoi(optarg);

			break;
		case '?':
			printf("\nWTF!!! Das ist Falsch!! \n\n Eine Moeglichkeit zum Ausfuehren des Programms waere: mpiexec -n 4 Closest_String_Problem.exe -v 3 -s 2 -f strings.txt\n\n");
			break;
		default:
			printf("hmmmmm\n");
		}
	}
}

// einlesen der Informationen aus der Quelle "strings.txt" Datei
void readFile(char* file) {

	FILE *source;

	// Bitte Pfad und Dateinamen anpassen
	source = fopen(file, "r");
	size_t len = 255;

	char* line = (char*)malloc(sizeof(char)*len);							// Speicher reservieren

	// prüfe ob die datei auch existiert
	if (source == NULL) {
		printf("Konnte Datei \"%s\" nicht oeffnen!\n", file);
		exit(2);
	}

	// 1. Zeile der Text datei auslesen und in eine Variabele speichern, mit fgets(ziel, n menge, source).
	if (fgets(line, len, source) != NULL) {

		if (input.count > (uint)atoi(line))								// von line in input.count geschrieben bzw verglichen
			input.count = (uint)atoi(line);

		input.values = (uint**)malloc(input.count * sizeof(uint*));		// reservieren für die ersten Pointer
	}

	// 2. Zeile der Text datei auslesen und in eine Variabele speichern, mit fgets(ziel, n menge, source).
	if (fgets(line, len, source) != NULL) {

		input.base = (uint)atoi(line);
	}

	int count = 0;
	while (fgets(line, len, source) != NULL) {

		if (input.length == -1)
			input.length = stringNrSize(line, input.base);

		// wandle hex-string zu uint um und schreibe in liste
		input.values[count] = string2Uint(line, input.base);				// jede zeile reservieren in der Funktion string2uint speicher

		count++;
		if (count >= input.count)											// die Anzahl an Strings die eingelesen werden aus der Text Datei
			break;
	}

	free(line);
}

//	Ermittelt die Gesamtmenge der möglichen strings anhand des Alphabets und der Stringlänge.
//	Dannach wird die Gesamtmenge in Teilmengen aufgeteilt in relatition der gesamt Prozesse die zur verfügung stehen.
//	Dabei wird der start Punkt und der end Punkt berechnet, wo díe Suche des besten String stattfindet.
uint* calcPoint(uint base, uint length, uint rank, uint numprocs) {
	uint* result = (uint*)malloc(length * sizeof(uint));
	int rest = 1;										// rest wird auf 1 gesetzt sonst ist das ergebnis der nachfolgende rechnung immer 0 
	for (int i = 0; i < length; i++) {					
		result[i] = ((rest * base) / numprocs) * rank;	// Berechnung der Ziffern die später den Startwert für runSolve festlegen. (curValue)
		rest = ((rest * base) % numprocs);				// Es wird der neue rest Berechnet mit Modula
	}													
	for (int i = length - 1; i > 0; i--) {				// es wird von der letzten Ziffer, jede stelle durchgangen.verglichen ob die ziffer größer oder gleich der Basis ist
		while (result[i] >= base) {						// vergleich von Ziffer mit der Basis wenn ziffer
			result[i - 1]++;							// wenn die ziffer größer als die Basis, damm erhöhe Ziffer auf der nächst kleineren Stelle um 1
			result[i] -= base;							// und subtrahiere von der aktuellen ziffer die Basis. die while schleife tut diese zwei schritte solange bis eine gpltige ziffer es gibt die auf der akt. Basis funktioniert
		}
	}
	return result;
}

//	Vergleich alle Werte zwischen start Punkt und end Punkt und Speichert dann das beste Ergebnis. 
void runSolve(uint* curValue, uint* endValue) {
	State s = newState(input.length, input.base, curValue, endValue);	// State ist der Aktuelle Zustand wo sich der Algorithmus gerade befindet.
	uint diff;

	while (1) {
		diff = compareWithInputValues(s.val, s.pos + 1);				// vergleich den Aktuellen Wert mit allen Werten die in der Text Datei vorhanden sind und speicher die distance in die variable diff.

		if (best.distance <= diff) {									// vergleich zwischen akt. diffenz mit der gerade berechneten distance. 
			if (stateValInc(&s) == 0)									// wenn beste Differenz kleiner gleich akt. Differenz, dann erhöhen den wert mit stateValInc.
				break;													// ausserdem gibt es stateValInc noch eine 0 oder eine 1 aus. Bei 0 ist stateValInc am endValue angekommen oder einen überlauf hat. 
			continue;
		}
		if (s.pos == s.length - 1) {									// überprüft das die Funktion über alle zeichen durchgelaufen ist, die der Länge der Strings aus der Text Datei entspechen.
			freeResult(&best);											// das bisherige bestes Ergebnis wird Freigegeben da ein neues ermittelt wurde
			best = newResult(s.length, s.val, diff);					// speicher das neue beste Ergebnis mit der Länge, wert und der Differenz
		}
		else {
			stateNext(&s);												// erhöht die Position
		}
	}
	freeState(&s);
}

//	Durchläuft iterativ die gegeben Strings und geschaut ob die Differenz (Hamming-Distanz) 
//	von allen gegebenen Strings zum neuen erstellten String minimal ist
uint compareWithInputValues(uint* s, uint length) {
	int worstDistance = 0;
	int distance;
	for (int i = 0; i < input.count; i++) {									// geht von 0 bis input.count(stringliste aus Text Datei) durch
		distance = hammingDistance(s, input.values[i], length);				// ermittelt die minimale Distanz von den gegeben strings zu den erstellten string
		if (distance > worstDistance)										// wenn Differnz höher ist als schlechteste Differenz
			worstDistance = distance;										// dann  Differenz in worstDifferenze hinein schreiben
	}
	return worstDistance;
}

//	Ermittelt die Distanz zwischen einen generierten String und einen gegebenen String vergleichen. 
//	Und dass jede Stelle der zeichenkette einzeln und bei ungleichen zahlen wird der Counter hoch gezählt.)
uint hammingDistance(uint* v1, uint* v2, uint length) {
	uint count = 0;
	for (int i = 0; i < length; i++)		// von 0 bis der länge der strings durch iteriert
		if (v1[i] != v2[i])					// jeder Index von dem gegebnen String mit den erstellten string verglichen
			count++;						// ungleicher inhalt dann counter +1 hochzählen
	return count;
}

// Funtkion uint2string konvertiert datentyp uint in einen Datentyp Char
char* uint2String(uint* nums, uint size) {						// übergabe an Funktion nums (Array) und länge der Strings
	char* result = (char*)calloc(size + 1, sizeof(char));		// speicher Reservierung für Char array

	char hex[] = "0\n";
	for (int i = 0; i < size; i++) {
		sprintf(hex, "%x\n", nums[i]);							// schreib die zahl (nums) in einen String (hex) rein nach dem schema	// num[0] = 5 // "5\n" -> hex
		result[i] = hex[0];										// speichern dann aus dem hex string in Result an die jeweilige stelle
	}
	return result;
}

// Funktion strungNrSize zum Zählen der Chars in einem String (stringlänge) von 0-9 oder Aa-fF. Es ist möglich das Basissystem bis Zz zu erhöhen.
uint stringNrSize(char* s, int base) {
	uint count = 0;

	char c = s[count];

	while ((c >= '0' && c <= '9') ||		// vergleich der Zeichen im String von 0-9
		(c >= 'a' && c <= 'f') ||			// vergleich der Zeichen im String von a-f
		(c >= 'A' && c <= 'F')) {			// vergleich der Zeichen im String von A-F
		count++;
		c = s[count];						// speichern das neue c ein

	}

	return count;

}

// Funktion string2Uint wandelt eine Zahl im String zu Ziffer Array um und gibt diese zurück
uint* string2Uint(char* string, uint base) {							// Übergebe den String und die Basis in die Funktion
	uint length = stringNrSize(string, base);							// Wird die Anzahl an Ziffern in einem String übergeben

	uint* result = (uint*)calloc(length, sizeof(uint));					// speicher Reservieren auch malloc möglich. Egal weil vor der zeitmessung.

	char tempVal[] = "0\n";												// erstellen Template String wo die erste Ziffer bearbeitet werden kann 
	for (int i = 0; i < length; i++) {									// Die komplette länge des Strings wird durch gegangen
		tempVal[0] = string[i];											// nimm aus dem String mit Index i charakter und setzt ihn auf die erste Stelle von TempString
		result[i] = (uint)strtoul(tempVal, NULL, base);					// mit strtoll: lies die zahl aus dem tempval ein mit der Basis(base). Cast in uint und speichert in result Array.
	}
	return result;														// wird als pointer wiederzurück gegeben
}

// Freigabe der Speicherreservierung Input Value
void freeInput(Input* input) {
	for (int i = 0; i < input->count; i++)			// wegen doppel Pointer muss jede unteradress auch freigemacht werden
		free(input->values[i]);
	free(input->values);							// dann die Komplette Speicherreservierung frei machen
}

// Freigabe der Speicherreservierung Result Value
void freeResult(Result* result) {
	free(result->value);
}

// Freigabe der Speicherreservierung State state
void freeState(State* state) {
	free(state->val);
	free(state->endVal);
}


// newResult wird nur für das beste Ergebenis benutzt. Das beste Ergebniss wird nur kopiert.
Result newResult(uint length, uint* value, uint distance) {
	Result r = { .length = length,								// Erstellung des Struct Result
				.distance = distance };						// Länge und Differenz wird aus dem Funktions kopf übernommen

	r.value = (uint*)malloc(length * sizeof(uint));				// Reservieren für value einen Adressenbereich
	for (int i = 0; i < length; ++i)							// Kopiert jede Ziffer von 0 bis Länge die Werte aus Value was im Funktionskof übergeben wurden 		
		r.value[i] = value[i];

	return r;
}

// Schaut ob aktueller Wert mit dem End Wet übereinstimmt
State newState(uint length, uint base, uint* curVal, uint* endVal) {
	State state = { .length = length,
					.base = base,
					.pos = 0,
					.endPos = -1,								// endPos = -1 damit es am amfamg nicht mit .pos übereinstimmt		
					.lastMove = 1 };


	state.val = (uint*)malloc(length * sizeof(uint));			// Speicher Reservierung für akt. Wert der als Array übergeben wird
	for (int i = 0; i < length; i++)							// Hier wird jede Ziffer seperate rüberkopiert. Von curVal nach state.val
		state.val[i] = curVal[i];

	state.endVal = (uint*)malloc(length * sizeof(uint));		// Speicher Reservierung für end Wert der als Array übergeben wird
	for (int i = 0; i < length; i++)							// Hier wird jede Ziffer seperate rüberkopiert. Von endVal nach state.endVal
		state.endVal[i] = endVal[i];

	while (state.pos < length - 1)								// Ist dafür da damit im Algorithmus nicht von Position 0 angefangen werden muss
		stateNext(&state);										// Es wird so lange ausgeführt bis es am letzten Zeichen des Strings angekommen ist.

	return state;
}


/*
	Erhöt aktuele vergleiswerte um ein abhängig von der aktuellen position
	und den überlauf des Zahlensystems
	returns:
		0: am ende angelangt
		1: wert mit aktueller position wurde erhöt
*/

//StateValInc erhöht den Zahlenwwert mit Überlauf abhängig von Zahlensystem. Diese Funktion imitiert andere Basissysteme.
int stateValInc(State* s) {
	s->lastMove = 1;											// annahme kein Abbruch kriterieum
	while (1) {
		s->val[s->pos]++;										// erhöhe die Ziffer auf akt. Pos.

		if (s->val[s->pos] >= s->base) {						// Ist Ziffer der akt. Pos. gleich der Basis(alphabets) 
			s->val[s->pos] = 0;									// falls das nicht der fall ist kommt es zum überlauf und die ziffer wird auf 0 gesetzt

			if (s->pos == 0 || s->endPos == (s->pos - 1)) {		// Abbruchkriterium: wenn das zutrifft das die akt. pos. = 0 (überlauf übern Überlauf) oder End pos. der akt. Pos. glech -1 
				s->lastMove = 0;
				break;
			}
			s->pos--;											// akt. Pos. runterzählen -1
		}
		else {													// Abbruchkriterium falls kein überlauf stattfindet
			break;
		}
	}
	return s->lastMove;
}

// Funktion stateNext vergleicht akt. Pos. mit der End Pos.
void stateNext(State* s) {
	if (s->val[s->pos] == s->endVal[s->pos]		// Vergleich akt. Ziffer mit End Ziffer
		&& s->endPos == (s->pos - 1))			// und schaut ob akt. Endposition  dem akt. Wert -1 entspricht
		s->endPos++;							// nur dann wird Endposition hochgezählt
	s->pos++;									// akt. Position immer hochzählen
}

// Erstellt eine Ausgabe der Ergebnisse und den Inhalt der Text Datei
void printResult() {
	if (best.distance == -1) {																		//Wenn die Beste Distance == -1 ist stimmt was nicht. da -1 bei unsigned uint eine sehr hohe zahl ist. also ist ein Fehler irgendwo passiert
		printf("No Result! Es ist eine Fehler passiert.\n");
		return;
	}

	printf("======================================\n");
	printf("NumCore: %d\n", numprocs);																// Printe Anzahl an ProzessorKernen mit denen Gerechnet wurde
	printf("base: %d\n", input.base);																// Printe die Basis/Alpahbetsystem
	printf("Closest string:\n");																	
	printf("Distance %u\n", best.distance);															// Printe die beste Distance die ermittelt wurde
	printf("New string: \n%*s\n", best.length, uint2String(best.value, best.length));				// Printe den besten Ermittelten String der gefunden wurde
	printf("%*s\tDistance\n", best.length, "String");

	uint dif;
	for (int i = 0; i < input.count; i++) {															// Printe alle Strings aus der Stringliste(TEXT Datei) und die Distance zu den besten String der Ermittelt wurde
		dif = hammingDistance(best.value, input.values[i], best.length);
		printf("%*s\t%d\n", best.length, uint2String(input.values[i], input.length), dif);
	}
	printf("======================================\n");
}
