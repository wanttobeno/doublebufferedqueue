
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <process.h>
#include "doublebufferedqueue.h"

struct DataPacket
{
	double elapsedTime;
	double series0;
	double series1;
};

class CDataView
{
	// The number of samples per data series used in this demo
	static const int m_nSampleSize = 10000;

	// The initial full range is set to 60 seconds of data.
	static const int m_nInitialFullRange = 60;

	// The maximum zoom in is 5 seconds.
	static const int m_nZoomInLimit = 5;

	double m_timeStamps[m_nSampleSize];	// The timestamps for the data series
	double m_dataSeriesA[m_nSampleSize];	// The values for the data series A
	double m_dataSeriesB[m_nSampleSize];	// The values for the data series B

public:
	CDataView()
	{
		m_bStopThread = false;
		m_nCurrentIndex = 0;
		for (int i = 0; i < m_nSampleSize;i++)
		{
			m_timeStamps[i] = 0;
			m_dataSeriesA[i] = 0;
			m_dataSeriesB[i] = 0; 
		}
	}
	virtual ~CDataView()
	{

	}

	bool AddData( double elapsedTime, double series0, double series1)
	{
		DataPacket packet;
		packet.elapsedTime = elapsedTime;
		packet.series0 = series0;
		packet.series1 = series1;

		bool bRet = buffer.put(packet);
		return bRet;
	}
	void Update()
	{
		int count;
		DataPacket *packets;
		if ((count = buffer.get(&packets)) <= 0)
			return;

		// 数据量过大，移除旧的元素
		// if data arrays have insufficient space, we need to remove some old data.
		if (m_nCurrentIndex + count >= m_nSampleSize)
		{
			// For safety, we check if the queue contains too much data than the entire data arrays. If
			// this is the case, we only use the latest data to completely fill the data arrays.
			if (count > m_nSampleSize)
			{
				packets += count - m_nSampleSize;
				count = m_nSampleSize;
			}

			// Remove oldest data to leave space for new data. To avoid frequent removal, we ensure at
			// least 5% empty space available after removal. 
			int originalIndex = m_nCurrentIndex;
			m_nCurrentIndex = m_nSampleSize * 95 / 100 - 1;
			if (m_nCurrentIndex > m_nSampleSize - count)
				m_nCurrentIndex = m_nSampleSize - count;

			for (int i = 0; i < m_nCurrentIndex; ++i)
			{
				int srcIndex = i + originalIndex - m_nCurrentIndex;
				m_timeStamps[i] = m_timeStamps[srcIndex];
				m_dataSeriesA[i] = m_dataSeriesA[srcIndex];
				m_dataSeriesB[i] = m_dataSeriesB[srcIndex];
			}
		}

		// Append the data from the queue to the data arrays
		for (int n = 0; n < count; ++n)
		{
			m_timeStamps[m_nCurrentIndex] = packets[n].elapsedTime;
			m_dataSeriesA[m_nCurrentIndex] = packets[n].series0;
			m_dataSeriesB[m_nCurrentIndex] = packets[n].series1;
			++m_nCurrentIndex;
		}

		//
		// As we added more data, we may need to update the full range of the viewport. 
		//

		double startDate = m_timeStamps[0];
		double endDate = m_timeStamps[m_nCurrentIndex - 1];

		// Use the initialFullRange (which is 60 seconds in this demo) if this is sufficient.
		double duration = endDate - startDate;
		if (duration < m_nInitialFullRange)
			endDate = startDate + m_nInitialFullRange;
	}

#define  MAC_ITEMS_SHOW 5
	void Print()
	{
		if (m_nCurrentIndex < MAC_ITEMS_SHOW)
			return;
		int nCurrent = m_nCurrentIndex;
		int nStart = max(0, nCurrent - MAC_ITEMS_SHOW);
		
		printf("\tTime\t\tValueA\t\tValueB\t\n");
		for (; nStart < nCurrent; nStart++)
		{
			printf("%15lf\t%15lf\t%15lf\t", m_timeStamps[nStart], m_dataSeriesA[nStart], m_dataSeriesB[nStart]);
			printf("\n");
		}
		printf("\n");
	}
public:
	bool m_bStopThread;
protected:
	DoubleBufferedQueue<DataPacket> buffer;
private:
	// The index of the array position to which new data values are added.
	int m_nCurrentIndex;
};

const int interval = 100;
unsigned __stdcall UpdataData(void * pParam)
{
	CDataView* self = (CDataView*)pParam;

	bool isFirst = true;
	LARGE_INTEGER startTime;
	LARGE_INTEGER currentTime;
	LARGE_INTEGER nextTime;
	LARGE_INTEGER frequency;
	QueryPerformanceFrequency(&frequency);

	// Random walk variables
	srand(999);
	double series0 = 32;
	double series1 = 63;
	double upperLimit = 94;
	double scaleFactor = sqrt(interval * 0.1);

	while (!self->m_bStopThread)
	{
		// Get current elapsed time
		QueryPerformanceCounter(&currentTime);
		if (isFirst)
		{
			startTime = nextTime = currentTime;
			isFirst = false;
		}

		// Compute the next data value
		double elapsedTime = (currentTime.QuadPart - startTime.QuadPart) / (double)frequency.QuadPart;
		if ((series0 = fabs(series0 + (rand() / (double)RAND_MAX - 0.5) * scaleFactor)) > upperLimit)
			series0 = upperLimit * 2 - series0;
		if ((series1 = fabs(series1 + (rand() / (double)RAND_MAX - 0.5) * scaleFactor)) > upperLimit)
			series1 = upperLimit * 2 - series1;

		// Call the handler
		//self->handler(self->param, elapsedTime, series0, series1);
		self->AddData(elapsedTime, series0, series1);

		// Sleep until next walk
		if ((nextTime.QuadPart += frequency.QuadPart * interval / 1000) <= currentTime.QuadPart)
			nextTime.QuadPart = currentTime.QuadPart + frequency.QuadPart * interval / 1000;

		int nSleepTime = (int)((nextTime.QuadPart - currentTime.QuadPart) * 1000 / frequency.QuadPart);
		::Sleep(nSleepTime);
	}

	return 0;
}


int main(int agrc, char** argv)
{
	CDataView  dataView;
	unsigned threadID;
	HANDLE thread = (HANDLE)_beginthreadex(NULL, 0, &UpdataData, &dataView, 0, &threadID);
	do 
	{
		dataView.Update();
		dataView.Print();
		Sleep(1000);
	} while (1);
	return 0;
}

