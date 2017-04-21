#pragma once

#include <types_def.h>
#include "rapid.h"

namespace rapidlib
{

	static const r_uint32 MINIMUM_LIST_SIZE = 8;

	/**
	* @class single_producer_consumer
	* @brief A single producer consumer implementation without critical sections.
	*/
	template<class SingleProducerConsumerType>
	class single_producer_consumer
	{
	public:
		single_producer_consumer();
		~single_producer_consumer();

		/**
		* @brief WriteLock must be immediately followed by WriteUnlock. These two functions must be called
		*        in the same thread.
		* @return A pointer to a block of data you can write to.
		*/
		SingleProducerConsumerType* WriteLock(void);

		/**
		* @brief Call when you are done writing to a block of memory returned by WriteLock()
		*/
		void WriteUnlock(void);

		/**
		* @brief ReadLock must be immediately followed by ReadUnlock. These two functions must be called
		*        in the same thread.
		* @return NULL if no data is availble to read, otherwise a pointer to a block of data you
		*         can read from.
		*/
		SingleProducerConsumerType* ReadLock(void);

		/**
		* @brief Signals that we are done reading the data from the least recent call of ReadLock.
		*        At this point that pointer is no longer valid, and should no longer be read.
		*/
		void ReadUnlock(void);

		/**
		* @brief ClearAll is not thread-safe and none of the lock or unlock functions should be called while it is running.
		*/
		void ClearAll(void);

	private:
		struct DataPlusPtr
		{
			DataPlusPtr(void) { mReadyToRead = false; }
			SingleProducerConsumerType          mObject;
			volatile r_bool                       mReadyToRead;
			volatile DataPlusPtr*               mNext;
		};
		volatile DataPlusPtr*                   mReadAheadPointer;
		volatile DataPlusPtr*                   mWriteAheadPointer;
		volatile DataPlusPtr*                   mReadPointer;
		volatile DataPlusPtr*                   mWritePointer;
		r_uint32                                 mReadCount;
		r_uint32                                 mWriteCount;
	};


	//=============================================================================
	// - class single_producer_consumer
	//=============================================================================
	template<class SingleProducerConsumerType>
	single_producer_consumer<SingleProducerConsumerType>::single_producer_consumer(void)
	{
		mReadPointer = new DataPlusPtr;
		mWritePointer = mReadPointer;
		mReadPointer->mNext = new DataPlusPtr;
		for (r_uint32 nCount = 2; nCount < MINIMUM_LIST_SIZE; ++nCount)
		{
			mReadPointer = mReadPointer->mNext;
			mReadPointer->mNext = new DataPlusPtr;
		}
		mReadPointer->mNext->mNext = mWritePointer;
		mReadPointer = mWritePointer;
		mReadAheadPointer = mReadPointer;
		mWriteAheadPointer = mWritePointer;
		mReadCount = mWriteCount = 0;
	}

	template<class SingleProducerConsumerType>
	single_producer_consumer<SingleProducerConsumerType>::~single_producer_consumer(void)
	{
		volatile DataPlusPtr* pNext = NULL;
		mReadPointer = mWriteAheadPointer->mNext;
		while (mReadPointer != mWriteAheadPointer)
		{
			pNext = mReadPointer->mNext;
			delete (char*)mReadPointer;
			mReadPointer = pNext;
		}
		delete (char*)mReadPointer;
	}

	template<class SingleProducerConsumerType>
	SingleProducerConsumerType* single_producer_consumer<SingleProducerConsumerType>::WriteLock(void)
	{
		if (mWriteAheadPointer->mNext == mReadPointer || mWriteAheadPointer->mNext->mReadyToRead)
		{
			volatile DataPlusPtr* pOriNext = mWriteAheadPointer->mNext;
			mWriteAheadPointer->mNext = new DataPlusPtr;
			r_assert(mWriteAheadPointer->mNext != NULL);
			mWriteAheadPointer->mNext->mNext = pOriNext;
		}
		volatile DataPlusPtr* pLast = mWriteAheadPointer;
		mWriteAheadPointer = mWriteAheadPointer->mNext;
		return (SingleProducerConsumerType*)pLast;
	}

	template<class SingleProducerConsumerType>
	void single_producer_consumer<SingleProducerConsumerType>::WriteUnlock(void)
	{
		mWriteCount++;
		mWritePointer->mReadyToRead = true;
		mWritePointer = mWritePointer->mNext;
	}

	template<class SingleProducerConsumerType>
	SingleProducerConsumerType* single_producer_consumer<SingleProducerConsumerType>::ReadLock(void)
	{
		if (mReadAheadPointer == mWritePointer || !mReadAheadPointer->mReadyToRead)
		{
			return NULL;
		}
		volatile DataPlusPtr* pLast = mReadAheadPointer;
		mReadAheadPointer = mReadAheadPointer->mNext;
		return (SingleProducerConsumerType*)pLast;
	}

	template<class SingleProducerConsumerType>
	void single_producer_consumer<SingleProducerConsumerType>::ReadUnlock(void)
	{
		mReadCount++;
		mReadPointer->mReadyToRead = false;
		mReadPointer = mReadPointer->mNext;
	}

	template<class SingleProducerConsumerType>
	void single_producer_consumer<SingleProducerConsumerType>::ClearAll(void)
	{
		volatile DataPlusPtr* pNext = NULL;
		mWritePointer = mReadPointer->mNext;
		int nCount = 1; pNext = mReadPointer->mNext;
		while (pNext != mReadPointer)
		{
			nCount++;
			pNext = pNext->mNext;
		}
		while (nCount-- > MINIMUM_LIST_SIZE)
		{
			pNext = mWritePointer->mNext;
			delete (char*)mWritePointer;
			mWritePointer = pNext;
		}
		mReadPointer->mNext = mWritePointer;
		mWritePointer = mReadPointer;
		mReadAheadPointer = mReadPointer;
		mWriteAheadPointer = mWritePointer;
		mReadCount = mWriteCount = 0;
	}

}
