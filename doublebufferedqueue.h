
#ifndef DOUBLEBUFFEREDQUEUE_HDR
#define DOUBLEBUFFEREDQUEUE_HDR

// CRITICAL_SECTION
#include <windows.h>

template <class T>
class DoubleBufferedQueue
{
public:

    //
    // Constructor - construct a queue with the given buffer size
    //
	DoubleBufferedQueue(int bufferSize = 10000) :
		m_bufferLen(0), m_bufferSize(bufferSize)
	{ 
		m_pBuffer0 = m_pCurrentBuffer = new T[bufferSize]; 
		m_pBuffer1 = new T[bufferSize];
		InitializeCriticalSection(&m_lock);
	}

    //
    // Destructor
    //
	~DoubleBufferedQueue()
	{
		 DeleteCriticalSection(&m_lock);
		 delete[] m_pBuffer0;
		 delete[] m_pBuffer1;
	}

    //
    // Add an item to the queue. Returns true if successful, false if the buffer is full.
    //
	bool put(const T& datum)
	{
		EnterCriticalSection(&m_lock);
		bool canWrite = m_bufferLen < m_bufferSize;
		if (canWrite) 
			m_pCurrentBuffer[m_bufferLen++] = datum;
		LeaveCriticalSection(&m_lock);
		return canWrite;
	}

    //
    // Get all the items in the queue. The T** argument should be the address of a variable to
    // receive the pointer to the item array. The return value is the size of the array.
    //
	int get(T** data)
	{
		EnterCriticalSection(&m_lock);
		*data = m_pCurrentBuffer;
		int ret = m_bufferLen;
		// Important !!!
		m_pCurrentBuffer = (m_pCurrentBuffer == m_pBuffer0) ? m_pBuffer1 : m_pBuffer0;
		m_bufferLen = 0;
		LeaveCriticalSection(&m_lock);
		return ret;
	}

private:

    // Disable copying and assignment
    DoubleBufferedQueue & operator=(const DoubleBufferedQueue&);
    DoubleBufferedQueue(const DoubleBufferedQueue&);

	T* m_pBuffer0;
	T* m_pBuffer1;
	T* m_pCurrentBuffer;
	int m_bufferLen;
	int m_bufferSize;
	CRITICAL_SECTION m_lock;
};


#endif
