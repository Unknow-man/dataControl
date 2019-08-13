#include "stdafx.h"
#include "stdlib.h"
#include "Def.h" 
#include "detectPVB.h"
#include <math.h>
#include <string.h>
#include <stdbool.h>
#include "heart_rate.h" 
#include "common.h" 

//ȫ�ֱ���

extern int BeatTemplates[MAXTYPES][BEATLGTH];
extern int BeatCounts[MAXTYPES];

int waveDectect(int *signal_in, int *peaks, int *valleys, int len);
int peakDect(int *y, int *valleys, int len);
int valleyDect(int *signal_in, int *valleys, int *peaks, int numPeak);
int lowpass5(int *ppg, int *ppg_lowpass,int len);
/* Calculate mean number */


//�Կ�Ư�ƺ��ppg������
int  ppg_RealAnalysisFile(int * ppg,  int * noiselevel,int len)
{
	int i,j, k, beatType = -1;
	int width = 101, step = 5;
	int max;
	int maxnumpeak,numpeak;
	static int BeatBuffer[ECG_BUFFER_LENGTH] = { 0 };
	int *Peak_Dot = NULL;
	int *Valley_Dot = NULL;

//	int *ppg_lowpass = NULL;
	maxnumpeak = (5 * len) / SAMPLE_RATE;         //�������Ϊ300
//	ppg_lowpass = MakeArray1D(len);
	Peak_Dot = MakeArray1D(200);
	Valley_Dot = MakeArray1D(200);

/*
	j = lowpass5(ppg, ppg_lowpass, len);                                //  ����5hz�ĵ�ͨ�˲���

	char *sout = (char *)(malloc(sizeof(char) * 512));
	FILE *fp;
	sout = "D://ppg_low.txt";
	fp = fopen(sout, "w");

	if (fp == NULL) {
		printf("Can't save file.");
	}


	for (int i = 0; i < anlylen; i++) {
		fprintf(fp, " %d\n", ppg_lowpass[i]);
	}
	fclose(fp);
*/
	//  ppg��originppg����������Ư�ƺ���ź�

	numpeak = waveDectect(ppg, Peak_Dot, Valley_Dot, len);


	if (numpeak < len / (2 * SAMPLE_RATE) || numpeak > 3 * len /  SAMPLE_RATE) {                     // ������������ʵ���30bmp���ߴ���180bmp����Ϊ����������
		*noiselevel = 1000;
		goto sign;
	}

	ResetBDAC();
    //�и�15���ppgΪnumpeak�Σ��Բ���Ϊ���ģ�����Peak_Dot��2/5λ��
	//��Peak_Dot�������ģ����40���㣬�ұ�59����
//	centralPoint = 2 * BEATLGTH / 5;
	for (k = 0; k < numpeak; k++) {
		if (Peak_Dot[k] < centralPoint || Peak_Dot[k] >= len - (BEATLGTH - centralPoint))
			continue;                                              //ִ����һ��k

		for (i = 0; i < ECG_BUFFER_LENGTH; ++i) {	
			BeatBuffer[i] = ppg[Peak_Dot[k] - centralPoint + i] - ppg[Peak_Dot[k]];
		}

		beatType = detectPVB(BeatBuffer, 0);
	}

	max = BeatCounts[0];
	for (i = 1; i < 8; ++i)
		if (BeatCounts[i] > max)
			max = BeatCounts[i];

	*noiselevel = (1000 * (numpeak - max)) / numpeak;
sign:
//	FreeArray1D(ppg_lowpass);
	FreeArray1D(Peak_Dot);
	FreeArray1D(Valley_Dot);

	return numpeak;
}

int waveDectect(int *signal_in, int *peaks, int *valleys, int len) {
	int i, numValley, numPeak, num , numNewValley;
	int *y;

	y = MakeArray1D(len);

	for (i = 0; i < len; i++)
		y[i] = -signal_in[i];   //  ����Ƿ壬ʵ���ǹ�
	numValley = peakDect(y, valleys, len);
	if (numValley > 1) {
		numPeak = numValley - 1;
	}
	else {
		numPeak = 0;
	}
	
	num = valleyDect(signal_in, valleys, peaks, numPeak);
	numNewValley = numPeak - 1;
	num = valleyDect(y, peaks, &valleys[1], numNewValley);
	FreeArray1D(y);

	return numPeak;
}

int peakDect(int *y, int *peaks, int len) {
	int i, k, max = 0, lastDatum = 0, temp;
	int timeSinceMax = 0, timeSinceLastPeak = 0;
	int  numPeak = 0, numValley = 0;
	bool ispeak;

	for (i = 0; i < len; i++) {
		if (timeSinceMax > 0)
			++timeSinceMax;

		if ((y[i] > lastDatum) && (y[i] > max))
		{
			max = y[i];
// 			if (max > 20)    �����С�߶�         
			timeSinceMax = 1;
		}
		else if (y[i]  < (max >> 1))
		{
			if (timeSinceMax > 0) {
				if (numPeak == 0)
					timeSinceLastPeak = i;
				else
					timeSinceLastPeak = i - peaks[numPeak - 1];
				//������֮������400ms���ϣ�ͬʱ��һ�����λ�ô���40
				if (timeSinceLastPeak > 40) {
					ispeak = true;
					temp = i - timeSinceMax + 1;
					for (k = temp - 10; k <= temp + 10; k++) {
						if (y[k] > y[temp]) {
							ispeak = false;
							break;
						}
					}
					if (ispeak) {
						peaks[numPeak] = i - timeSinceMax + 1;
						numPeak++;
					}
				}
			}
			max = 0;
			timeSinceMax = 0;
		}
		else if (timeSinceMax > 40)
		{
			ispeak = true;
			temp = i - timeSinceMax + 1;
			for (k = temp - 10; k <= temp + 10; k++) {
				if (y[k] > y[temp]) {
					ispeak = false;
					break;
				}
			}
			if (ispeak) {
				peaks[numPeak] = i - timeSinceMax + 1;
				numPeak++;
			}
			max = 0;
			timeSinceMax = 0;
		}

		lastDatum = y[i];
	}

	return numPeak;
}
int valleyDect(int *signal_in, int *Valley_Dot, int *Peak_Dot, int numPeak) {
	int i, j, m_peak = 0;
	int PeakData;

	for (i = 0; i < numPeak; i++) {
		PeakData = signal_in[Valley_Dot[i]];

		Peak_Dot[m_peak] = Valley_Dot[i] + 9;
		for (j = Valley_Dot[i] + 10; j <= Valley_Dot[i + 1] - 10; j++) {              
			if (signal_in[j] >= PeakData) {
	
				PeakData = signal_in[j];
				Peak_Dot[m_peak] = j;
			}
		}
		m_peak++;
	}

	return m_peak;
}
int mean(int *vin, int width, int step)
{
	int sum;
	int i;

	for (i = 0, sum = 0; i < width; i += step)
		sum += vin[i];
	sum /= (width / step) + 1;      //  ����vin�ľ�ֵ

	return (sum);
}
int lowpass5(int *ppg, int *ppg_lowpass,int len)
{
	int i,k;
	double sum ;
//	const double b[8] = { 0.0174063246944534 ,	0.0612073874532602	, 0.166164405704519 ,	0.255221882147768 ,	0.255221882147768 ,	0.166164405704519 ,	0.0612073874532602 ,	0.0174063246944534 };
	const double b[8] = { 0.0174 ,	0.0612	, 0.166 ,	0.255 ,	0.255 ,	0.166 ,	0.0612 ,	0.0174};
	for (k = 0; k < len - 7; k++) {
		for (i = 0 ,sum = 0.0; i < 8; i++) {
			sum += b[i] * ppg[k + i];
		}
		ppg_lowpass[k + 7] = (int)sum;
	}
	for (k = 0; k < 7; k++) {
		ppg_lowpass[k] = ppg_lowpass[7];
	}
	return 1;
}


int * MakeArray1D(int col)
{
	int *out;
	out = (int *)YxAppMemMalloc(sizeof(int) * col);
	if (out == NULL)
	{
		lemprintf("can't malloc in MakeArray1D\n");
		return NULL;
	}
	return (out);
}

void FreeArray1D(int * in)
{
	if(in)
		lemfree(in);
}