/* -------------------------------------------------------------------------------
 * Copyright (c) 2018, OLogN Technologies AG
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the <organization> nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * -------------------------------------------------------------------------------
 * 
 * Per-thread bucket allocator
 * 
 * v.1.00    May-09-2018    Initial release
 * 
 * -------------------------------------------------------------------------------*/


#include "random_test.h"

thread_local unsigned long long rnd_seed = 0;


NOINLINE
void randomPos_FixedSize_UsingPerThreadAllocatorExp( size_t iterCount, size_t maxItemsExp, size_t maxItemSizeExp, size_t threadID )
{
	const size_t SIZE = 1024 * 1024 * 1024;
	g_AllocManager.initialize( SIZE );
	g_AllocManager.enable();

	size_t maxItems = ((size_t)1) << maxItemsExp;
	size_t itemIdxMask = maxItems - 1;
	size_t itemSize = ((size_t)1) << maxItemSizeExp;

	uint8_t** baseBuff = reinterpret_cast<uint8_t**>( g_AllocManager.allocate( maxItems * sizeof(uint8_t*) ) );
	memset( baseBuff, 0, maxItems * sizeof( uint8_t* ) );
	for ( size_t i=0;i<iterCount; ++i )
	{
		size_t randNum = rng();
		size_t idx = randNum & itemIdxMask;
		if ( baseBuff[idx] )
		{
			g_AllocManager.deallocate( baseBuff[idx] );
			baseBuff[idx] = 0;
		}
		else
		{
			baseBuff[idx] = reinterpret_cast<uint8_t*>( g_AllocManager.allocate( itemSize ) );
		}
	}
	for ( size_t idx=0; idx<maxItems; ++idx )
		if ( baseBuff[idx] )
			g_AllocManager.deallocate( baseBuff[idx] );

	g_AllocManager.deallocate( baseBuff );
	g_AllocManager.printStats();
	g_AllocManager.disable();
		
	printf( "about to exit thread %zd (%zd operations performed)...\n", threadID, iterCount );
}

NOINLINE
void randomPos_FixedSize_UsingNewAndDeleteExp( size_t iterCount, size_t maxItemsExp, size_t maxItemSizeExp, size_t threadID )
{
	size_t maxItems = ((size_t)1) << maxItemsExp;
	size_t itemIdxMask = maxItems - 1;
	size_t itemSize = ((size_t)1) << maxItemSizeExp;
	uint8_t** baseBuff = new uint8_t* [maxItems];
	memset( baseBuff, 0, maxItems * sizeof( uint8_t* ) );
	for ( size_t i=0;i<iterCount; ++i )
	{
		size_t randNum = rng();
		size_t idx = randNum & itemIdxMask;
		if ( baseBuff[idx] )
		{
			delete [] baseBuff[idx];
			baseBuff[idx] = 0;
		}
		else
		{
			baseBuff[idx] = new uint8_t[ itemSize ];
		}
	}
	for ( size_t idx=0; idx<maxItems; ++idx )
		if ( baseBuff[idx] )
			delete [] baseBuff[idx];
	delete [] baseBuff;
	printf( "about to exit thread %zd (%zd operations performed)...\n", threadID, iterCount );
}

NOINLINE
void randomPos_FixedSize_EmptyExp( size_t iterCount, size_t maxItemsExp, size_t maxItemSizeExp, size_t threadID )
{
	size_t maxItems = ((size_t)1) << maxItemsExp;
	size_t itemIdxMask = maxItems - 1;
	size_t itemSize = ((size_t)1) << maxItemSizeExp;
	uint8_t** baseBuff = new uint8_t* [maxItems];
	memset( baseBuff, 0, maxItems * sizeof( uint8_t* ) );
	for ( size_t i=0;i<iterCount; ++i )
	{
		size_t randNum = rng();
		size_t idx = randNum & itemIdxMask;
		if ( baseBuff[idx] )
		{
			baseBuff[idx] = 0;
		}
		else
		{
			baseBuff[idx] = reinterpret_cast<uint8_t*>(itemSize);
		}
	}
	delete [] baseBuff;
	printf( "about to exit thread %zd (%zd operations performed)...\n", threadID, iterCount );
}


NOINLINE
void randomPos_FixedSize_FullMemAccess_UsingPerThreadAllocatorExp( size_t iterCount, size_t maxItemsExp, size_t maxItemSizeExp, size_t threadID )
{
	const size_t SIZE = 1024 * 1024 * 1024;
	g_AllocManager.initialize( SIZE );
	g_AllocManager.enable();

	size_t maxItems = ((size_t)1) << maxItemsExp;
	size_t itemIdxMask = maxItems - 1;
	size_t itemSize = ((size_t)1) << maxItemSizeExp;
	assert( itemSize >= sizeof( size_t ) );

	size_t dummyCtr = 0;
	 
	uint8_t** baseBuff = reinterpret_cast<uint8_t**>( g_AllocManager.allocate( maxItems * sizeof(uint8_t*) ) );
	memset( baseBuff, 0, maxItems * sizeof( uint8_t* ) );

	size_t distanceMeasure = 0;
	uint8_t* prev = reinterpret_cast<uint8_t*>( baseBuff );
	size_t nowAllocated = maxItems * sizeof( uint8_t* );
	size_t maxAllocated = nowAllocated;

	size_t currentItems = 0;
	size_t maxCurrentItems = 0;

	for ( size_t i=0;i<iterCount; ++i )
	{
		size_t randNum = rng();
		size_t idx = randNum & itemIdxMask;

		if (currentItems > maxCurrentItems)
			maxCurrentItems = currentItems;

		if ( baseBuff[idx] )
		{
			for ( size_t i=0; i<itemSize/sizeof(size_t); ++i )
				dummyCtr += ( reinterpret_cast<size_t*>( baseBuff[idx] ) )[i];
			g_AllocManager.deallocate( baseBuff[idx] );
			if ( baseBuff[idx] <= prev )
				distanceMeasure += prev - baseBuff[idx];
			else
				distanceMeasure += baseBuff[idx] - prev;
			prev = baseBuff[idx];
			baseBuff[idx] = 0;
			assert( nowAllocated >= itemSize );
			nowAllocated -= itemSize;
			--currentItems;
		}
		else
		{
			baseBuff[idx] = reinterpret_cast<uint8_t*>( g_AllocManager.allocate( itemSize ) );
			memset( baseBuff[idx], (uint8_t)(idx), itemSize );
			if ( baseBuff[idx] <= prev )
				distanceMeasure += prev - baseBuff[idx];
			else
				distanceMeasure += baseBuff[idx] - prev;
			prev = baseBuff[idx];
			nowAllocated += itemSize;
			if ( maxAllocated < nowAllocated )
				maxAllocated = nowAllocated;
			++currentItems;
		}
	}
	for ( size_t idx=0; idx<maxItems; ++idx )
		if ( baseBuff[idx] )
		{
//			fmt::print("{}\n", static_cast<void*>(baseBuff[idx]));
			for ( size_t i=0; i<itemSize/sizeof(size_t); ++i )
				dummyCtr += ( reinterpret_cast<size_t*>( baseBuff[idx] ) )[i];
			g_AllocManager.deallocate( baseBuff[idx] );
			if ( baseBuff[idx] <= prev )
				distanceMeasure += prev - baseBuff[idx];
			else
				distanceMeasure += baseBuff[idx] - prev;
			prev = baseBuff[idx];
			assert( nowAllocated >= itemSize );
			nowAllocated -= itemSize;
		}

	g_AllocManager.deallocate( baseBuff );
	g_AllocManager.printStats();
	g_AllocManager.disable();
	assert( nowAllocated == maxItems * sizeof( uint8_t* ) );
	nowAllocated -= maxItems * sizeof( uint8_t* );
		
	printf( "about to exit thread %zd (%zd operations performed) [ctr = %zd] distanceMeasure = %zd; nowAllocated = %zd, maxAllocated = %zd, maxCurrentItems = %zd...\n", threadID, iterCount, dummyCtr, distanceMeasure, nowAllocated, maxAllocated, maxCurrentItems);
}

NOINLINE
void randomPos_FixedSize_FullMemAccess_UsingNewAndDeleteExp( size_t iterCount, size_t maxItemsExp, size_t maxItemSizeExp, size_t threadID )
{
	size_t maxItems = ((size_t)1) << maxItemsExp;
	size_t itemIdxMask = maxItems - 1;
	size_t itemSize = ((size_t)1) << maxItemSizeExp;
	assert( itemSize >= sizeof( size_t ) );

	size_t dummyCtr = 0;

	uint8_t** baseBuff = new uint8_t* [maxItems];
	memset( baseBuff, 0, maxItems * sizeof( uint8_t* ) );

	size_t distanceMeasure = 0;
	uint8_t* prev = reinterpret_cast<uint8_t*>( baseBuff );

	for ( size_t i=0;i<iterCount; ++i )
	{
		size_t randNum = rng();
		size_t idx = randNum & itemIdxMask;
		if ( baseBuff[idx] )
		{
			for ( size_t i=0; i<itemSize/sizeof(size_t); ++i )
				dummyCtr += ( reinterpret_cast<size_t*>( baseBuff[idx] ) )[i];
			delete [] baseBuff[idx];
			if ( baseBuff[idx] <= prev )
				distanceMeasure += prev - baseBuff[idx];
			else
				distanceMeasure += baseBuff[idx] - prev;
			prev = baseBuff[idx];
			baseBuff[idx] = 0;
		}
		else
		{
			baseBuff[idx] = new uint8_t[ itemSize ];
			memset( baseBuff[idx], (uint8_t)(idx), itemSize );
			if ( baseBuff[idx] <= prev )
				distanceMeasure += prev - baseBuff[idx];
			else
				distanceMeasure += baseBuff[idx] - prev;
			prev = baseBuff[idx];
		}
	}
	for ( size_t idx=0; idx<maxItems; ++idx )
		if ( baseBuff[idx] )
		{
//			fmt::print("{}\n", static_cast<void*>(baseBuff[idx]));
			for ( size_t i=0; i<itemSize/sizeof(size_t); ++i )
				dummyCtr += ( reinterpret_cast<size_t*>( baseBuff[idx] ) )[i];
			if ( baseBuff[idx] <= prev )
				distanceMeasure += prev - baseBuff[idx];
			else
				distanceMeasure += baseBuff[idx] - prev;
			prev = baseBuff[idx];
			delete [] baseBuff[idx];
		}
	delete [] baseBuff;
	printf( "about to exit thread %zd (%zd operations performed) [ctr = %zd] distanceMeasure = %zd...\n", threadID, iterCount, dummyCtr, distanceMeasure );
}

NOINLINE
void randomPos_FixedSize_FullMemAccess_EmptyExp( size_t iterCount, size_t maxItemsExp, size_t maxItemSizeExp, size_t threadID )
{
	size_t maxItems = ((size_t)1) << maxItemsExp;
	size_t itemIdxMask = maxItems - 1;
	size_t itemSize = ((size_t)1) << maxItemSizeExp;
	uint8_t** baseBuff = new uint8_t* [maxItems];
	memset( baseBuff, 0, maxItems * sizeof( uint8_t* ) );
	for ( size_t i=0;i<iterCount; ++i )
	{
		size_t randNum = rng();
		size_t idx = randNum & itemIdxMask;
		if ( baseBuff[idx] )
		{
			baseBuff[idx] = 0;
		}
		else
		{
			baseBuff[idx] = reinterpret_cast<uint8_t*>(itemSize);
		}
	}
	delete [] baseBuff;
	printf( "about to exit thread %zd (%zd operations performed)...\n", threadID, iterCount );
}


NOINLINE
void randomTestUsingPerThreadAllocatorExp( size_t iterCount, size_t maxItemsExp, size_t maxItemSizeExp, size_t threadID )
{
	const size_t SIZE = 1024 * 1024 * 1024;
//	g_AllocManager.initialize(  ((size_t)1) << ( 1 + maxItemsExp + maxItemSizeExp ) );
	g_AllocManager.initialize( SIZE );
	g_AllocManager.enable();

	size_t maxItems = ((size_t)1) << maxItemsExp;
	size_t itemIdxMask = maxItems - 1;
	size_t itemSizeMask = ( ((size_t)1) << maxItemSizeExp) - 1;

	uint8_t** baseBuff = reinterpret_cast<uint8_t**>( g_AllocManager.allocate( maxItems * sizeof(uint8_t*) ) );
	memset( baseBuff, 0, maxItems * sizeof( uint8_t* ) );
	for ( size_t i=0;i<iterCount; ++i )
	{
		size_t randNum = rng();
		size_t idx = randNum & itemIdxMask;
		if ( baseBuff[idx] )
		{
			g_AllocManager.deallocate( baseBuff[idx] );
			baseBuff[idx] = 0;
		}
		else
		{
			randNum >>= maxItemsExp; // let's be economical with randomicity!
			size_t sz = calcSizeWithStatsAdjustment( randNum, maxItemSizeExp );
			baseBuff[idx] = reinterpret_cast<uint8_t*>( g_AllocManager.allocate( sz ) );
		}
	}
	for ( size_t idx=0; idx<maxItems; ++idx )
		if ( baseBuff[idx] )
			g_AllocManager.deallocate( baseBuff[idx] );

	g_AllocManager.deallocate( baseBuff );
	g_AllocManager.printStats();
	g_AllocManager.disable();
		
	printf( "about to exit thread %zd (%zd operations performed)...\n", threadID, iterCount );
}

NOINLINE
void randomTestUsingNewAndDeleteExp( size_t iterCount, size_t maxItemsExp, size_t maxItemSizeExp, size_t threadID )
{
	size_t maxItems = ((size_t)1) << maxItemsExp;
	size_t itemIdxMask = maxItems - 1;
	size_t itemSizeMask = ( ((size_t)1) << maxItemSizeExp) - 1;
	uint8_t** baseBuff = new uint8_t* [maxItems];
	memset( baseBuff, 0, maxItems * sizeof( uint8_t* ) );
	for ( size_t i=0;i<iterCount; ++i )
	{
		size_t randNum = rng();
		size_t idx = randNum & itemIdxMask;
		if ( baseBuff[idx] )
		{
			delete [] baseBuff[idx];
			baseBuff[idx] = 0;
		}
		else
		{
			randNum >>= maxItemsExp; // let's be economical with randomicity!
			size_t sz = calcSizeWithStatsAdjustment( randNum, maxItemSizeExp );
			baseBuff[idx] = new uint8_t[ sz ];
		}
	}
	for ( size_t idx=0; idx<maxItems; ++idx )
		if ( baseBuff[idx] )
			delete [] baseBuff[idx];
	delete [] baseBuff;
	printf( "about to exit thread %zd (%zd operations performed)...\n", threadID, iterCount );
}

NOINLINE
void randomTestEmptyExp( size_t iterCount, size_t maxItemsExp, size_t maxItemSizeExp, size_t threadID )
{
	size_t maxItems = ((size_t)1) << maxItemsExp;
	size_t itemIdxMask = maxItems - 1;
	size_t itemSizeMask = ( ((size_t)1) << maxItemSizeExp) - 1;
	uint8_t** baseBuff = new uint8_t* [maxItems];
	memset( baseBuff, 0, maxItems * sizeof( uint8_t* ) );
	for ( size_t i=0;i<iterCount; ++i )
	{
		size_t randNum = rng();
		size_t idx = randNum & itemIdxMask;
		if ( baseBuff[idx] )
		{
			baseBuff[idx] = 0;
		}
		else
		{
			randNum >>= maxItemsExp; // let's be economical with randomicity!
			size_t sz = calcSizeWithStatsAdjustment( randNum, maxItemSizeExp );
			baseBuff[idx] = reinterpret_cast<uint8_t*>(sz);
		}
	}
	delete [] baseBuff;
	printf( "about to exit thread %zd (%zd operations performed)...\n", threadID, iterCount );
}


NOINLINE
void randomTest_DeallocAllocLeastRecentlyUsed_RandomUnitSize_UsingPerThreadAllocatorExp( size_t iterCount, size_t maxItemsExp, size_t maxItemSizeExp, size_t threadID )
{
	const size_t SIZE = 1024 * 1024 * 1024;
//	g_AllocManager.initialize(  ((size_t)1) << ( 1 + maxItemsExp + maxItemSizeExp ) );
	g_AllocManager.initialize( SIZE );
	g_AllocManager.enable();

	size_t maxItems = ((size_t)1) << maxItemsExp;
	size_t itemIdxMask = maxItems - 1;
	size_t itemSizeMask = ( ((size_t)1) << maxItemSizeExp) - 1;

	uint8_t** baseBuff = reinterpret_cast<uint8_t**>( g_AllocManager.allocate( maxItems * sizeof(uint8_t*) ) );

	// phase 1: set up
	for ( size_t i=0;i<maxItems; ++i )
	{
		size_t sz = calcSizeWithStatsAdjustment( rng(), maxItemSizeExp );
		baseBuff[i] = reinterpret_cast<uint8_t*>( g_AllocManager.allocate( sz ) );
		*reinterpret_cast<size_t*>(baseBuff[i]) = i;
	}
	// phase 2: dealloc least recently used and alloc it again
	for ( size_t i=maxItems;i<iterCount+maxItems; ++i )
	{
		size_t idx = i & itemIdxMask;
		assert( *reinterpret_cast<size_t*>(baseBuff[idx]) == i - maxItems );
		g_AllocManager.deallocate( baseBuff[idx] );
		size_t sz = calcSizeWithStatsAdjustment( rng(), maxItemSizeExp );
		baseBuff[idx] = reinterpret_cast<uint8_t*>( g_AllocManager.allocate( sz ) );
		*reinterpret_cast<size_t*>(baseBuff[idx]) = idx;
	}
	// phase 3: cleanup
	for ( size_t i=0;i<maxItems; ++i )
	{
		assert( *reinterpret_cast<size_t*>(baseBuff[i]) == iterCount + i );
		g_AllocManager.deallocate( baseBuff[i] );
	}

	g_AllocManager.deallocate( baseBuff );
	g_AllocManager.printStats();
	g_AllocManager.disable();
		
	printf( "about to exit thread %zd (%zd operations performed)...\n", threadID, iterCount );
}

NOINLINE
void randomTest_DeallocAllocLeastRecentlyUsed_RandomUnitSize_UsingNewAndDeleteExp( size_t iterCount, size_t maxItemsExp, size_t maxItemSizeExp, size_t threadID )
{
	size_t maxItems = ((size_t)1) << maxItemsExp;
	size_t itemIdxMask = maxItems - 1;
	size_t itemSizeMask = ( ((size_t)1) << maxItemSizeExp) - 1;
	uint8_t** baseBuff = new uint8_t* [maxItems];
	// phase 1: set up
	for ( size_t i=0;i<maxItems; ++i )
	{
		size_t sz = calcSizeWithStatsAdjustment( rng(), maxItemSizeExp );
		baseBuff[i] = new uint8_t[ sz ];
		*reinterpret_cast<size_t*>(baseBuff[i]) = i;
	}
	// phase 2: dealloc least recently used and alloc it again
	for ( size_t i=maxItems;i<iterCount+maxItems; ++i )
	{
		size_t idx = i & itemIdxMask;
		assert( *reinterpret_cast<size_t*>(baseBuff[idx]) == i - maxItems );
		delete [] baseBuff[idx];
		size_t sz = calcSizeWithStatsAdjustment( rng(), maxItemSizeExp );
		baseBuff[idx] = new uint8_t[ sz ];
		*reinterpret_cast<size_t*>(baseBuff[idx]) = idx;
	}
	// phase 3: cleanup
	for ( size_t i=0;i<maxItems; ++i )
	{
		assert( *reinterpret_cast<size_t*>(baseBuff[i]) == iterCount + i );
		delete [] baseBuff[i];
	}
	delete [] baseBuff;
	printf( "about to exit thread %zd (%zd operations performed)...\n", threadID, iterCount );
}

NOINLINE
void randomTest_DeallocAllocLeastRecentlyUsed_RandomUnitSize_EmptyExp( size_t iterCount, size_t maxItemsExp, size_t maxItemSizeExp, size_t threadID )
{
	size_t maxItems = ((size_t)1) << maxItemsExp;
	size_t itemIdxMask = maxItems - 1;
	size_t itemSizeMask = ( ((size_t)1) << maxItemSizeExp) - 1;
	uint8_t** baseBuff = new uint8_t* [maxItems];
	// phase 1: set up
	for ( size_t i=0;i<maxItems; ++i )
	{
		size_t sz = calcSizeWithStatsAdjustment( rng(), maxItemSizeExp );
		baseBuff[i] = reinterpret_cast<uint8_t*>( sz );
	}
	// phase 2: dealloc least recently used and alloc it again
	for ( size_t i=maxItems;i<iterCount+maxItems; ++i )
	{
		size_t idx = i & itemIdxMask;
		size_t sz = calcSizeWithStatsAdjustment( rng(), maxItemSizeExp );
		baseBuff[idx] = reinterpret_cast<uint8_t*>( sz );
	}
	// phase 3: cleanup
	delete [] baseBuff;
	printf( "about to exit thread %zd (%zd operations performed)...\n", threadID, iterCount );
}


NOINLINE
void randomTest_DeallocAllocLeastRecentlyUsed_SameUnitSize_UsingPerThreadAllocatorExp( size_t iterCount, size_t maxItemsExp, size_t maxItemSizeExp, size_t threadID )
{
	const size_t SIZE = 1024 * 1024 * 1024;
//	g_AllocManager.initialize(  ((size_t)1) << ( 1 + maxItemsExp + maxItemSizeExp ) );
	g_AllocManager.initialize( SIZE );
	g_AllocManager.enable();

	size_t maxItems = ((size_t)1) << maxItemsExp;
	size_t itemIdxMask = maxItems - 1;
	size_t itemSize = ( ((size_t)1) << maxItemSizeExp);

	uint8_t** baseBuff = reinterpret_cast<uint8_t**>( g_AllocManager.allocate( maxItems * sizeof(uint8_t*) ) );

	// phase 1: set up
	for ( size_t i=0;i<maxItems; ++i )
	{
		baseBuff[i] = reinterpret_cast<uint8_t*>( g_AllocManager.allocate( itemSize ) );
		*reinterpret_cast<size_t*>(baseBuff[i]) = i;
	}
	// phase 2: dealloc least recently used and alloc it again
	for ( size_t i=maxItems;i<iterCount+maxItems; ++i )
	{
		size_t idx = i & itemIdxMask;
		assert( *reinterpret_cast<size_t*>(baseBuff[idx]) == i - maxItems );
		g_AllocManager.deallocate( baseBuff[idx] );
		baseBuff[idx] = reinterpret_cast<uint8_t*>( g_AllocManager.allocate( itemSize ) );
		*reinterpret_cast<size_t*>(baseBuff[idx]) = idx;
	}
	// phase 3: cleanup
	for ( size_t i=0;i<maxItems; ++i )
	{
		assert( *reinterpret_cast<size_t*>(baseBuff[i]) == iterCount + i );
		g_AllocManager.deallocate( baseBuff[i] );
	}

	g_AllocManager.deallocate( baseBuff );
	g_AllocManager.printStats();
	g_AllocManager.disable();
		
	printf( "about to exit thread %zd (%zd operations performed)...\n", threadID, iterCount );
}

NOINLINE
void randomTest_DeallocAllocLeastRecentlyUsed_SameUnitSize_UsingNewAndDeleteExp( size_t iterCount, size_t maxItemsExp, size_t maxItemSizeExp, size_t threadID )
{
	size_t maxItems = ((size_t)1) << maxItemsExp;
	size_t itemIdxMask = maxItems - 1;
	size_t itemSize = ( ((size_t)1) << maxItemSizeExp);
	uint8_t** baseBuff = new uint8_t* [maxItems];
	// phase 1: set up
	for ( size_t i=0;i<maxItems; ++i )
	{
		baseBuff[i] = new uint8_t[ itemSize ];
		*reinterpret_cast<size_t*>(baseBuff[i]) = i;
	}
	// phase 2: dealloc least recently used and alloc it again
	for ( size_t i=maxItems;i<iterCount+maxItems; ++i )
	{
		size_t idx = i & itemIdxMask;
		assert( *reinterpret_cast<size_t*>(baseBuff[idx]) == i - maxItems );
		delete [] baseBuff[idx];
		baseBuff[idx] = new uint8_t[ itemSize ];
		*reinterpret_cast<size_t*>(baseBuff[idx]) = idx;
	}
	// phase 3: cleanup
	for ( size_t i=0;i<maxItems; ++i )
	{
		assert( *reinterpret_cast<size_t*>(baseBuff[i]) == iterCount + i );
		delete [] baseBuff[i];
	}
	delete [] baseBuff;
	printf( "about to exit thread %zd (%zd operations performed)...\n", threadID, iterCount );
}

NOINLINE
void randomTest_DeallocAllocLeastRecentlyUsed_SameUnitSize_EmptyExp( size_t iterCount, size_t maxItemsExp, size_t maxItemSizeExp, size_t threadID )
{
	size_t maxItems = ((size_t)1) << maxItemsExp;
	size_t itemIdxMask = maxItems - 1;
	size_t itemSize = ( ((size_t)1) << maxItemSizeExp);
	uint8_t** baseBuff = new uint8_t* [maxItems];
	// phase 1: set up
	for ( size_t i=0;i<maxItems; ++i )
	{
		baseBuff[i] = reinterpret_cast<uint8_t*>( itemSize );
	}
	// phase 2: dealloc least recently used and alloc it again
	for ( size_t i=maxItems;i<iterCount+maxItems; ++i )
	{
		size_t idx = i & itemIdxMask;
		baseBuff[idx] = reinterpret_cast<uint8_t*>( itemSize );
	}
	// phase 3: cleanup
	delete [] baseBuff;
	printf( "about to exit thread %zd (%zd operations performed)...\n", threadID, iterCount );
}


NOINLINE
void randomTest_FrequentAndInfrequent_RandomUnitSize_UsingPerThreadAllocatorExp( size_t iterCount, size_t maxFastItemsExp, size_t maxFastItemSizeExp, size_t maxSlowItemsExp, size_t maxSlowItemSizeExp, size_t threadID )
{
	const size_t SIZE = 1024 * 1024 * 1024;
	g_AllocManager.initialize( SIZE );
	g_AllocManager.enable();

	size_t maxFastItems = ((size_t)1) << maxFastItemsExp;
	size_t fastItemIdxMask = maxFastItems - 1;

	size_t maxSlowItems = ((size_t)1) << maxSlowItemsExp;
	size_t slowItemIdxMask = maxSlowItems - 1;

	uint8_t** baseBuffFast = reinterpret_cast<uint8_t**>( g_AllocManager.allocate( maxFastItems * sizeof(uint8_t*) ) );
	uint8_t** baseBuffSlow = reinterpret_cast<uint8_t**>( g_AllocManager.allocate( maxSlowItems * sizeof(uint8_t*) ) );

	memset( baseBuffFast, 0, maxFastItems * sizeof( uint8_t* ) );
	memset( baseBuffSlow, 0, maxSlowItems * sizeof( uint8_t* ) );

	for ( size_t i=0;i<iterCount; ++i )
	{
		{
			size_t randNum = rng();
			size_t idx = randNum & fastItemIdxMask;
			if ( baseBuffFast[idx] )
			{
				g_AllocManager.deallocate( baseBuffFast[idx] );
				baseBuffFast[idx] = 0;
			}
			else
			{
				randNum >>= maxFastItemsExp; // let's be economical with randomicity!
				size_t sz = calcSizeWithStatsAdjustment( randNum, maxFastItemSizeExp );
				baseBuffFast[idx] = reinterpret_cast<uint8_t*>( g_AllocManager.allocate( sz ) );
			}
		}
		{
			size_t randNum = rng();
			size_t idx = randNum & slowItemIdxMask;
			if ( baseBuffSlow[idx] )
			{
				g_AllocManager.deallocate( baseBuffSlow[idx] );
				baseBuffSlow[idx] = 0;
			}
			else
			{
				randNum >>= maxSlowItemsExp; // let's be economical with randomicity!
				size_t sz = calcSizeWithStatsAdjustment( randNum, maxSlowItemSizeExp );
				baseBuffSlow[idx] = reinterpret_cast<uint8_t*>( g_AllocManager.allocate( sz ) );
			}
		}
	}
	for ( size_t idx=0; idx<maxFastItems; ++idx )
		if ( baseBuffFast[idx] )
			g_AllocManager.deallocate( baseBuffFast[idx] );
	for ( size_t idx=0; idx<maxSlowItems; ++idx )
		if ( baseBuffSlow[idx] )
			g_AllocManager.deallocate( baseBuffSlow[idx] );

	g_AllocManager.deallocate( baseBuffSlow );
	g_AllocManager.deallocate( baseBuffFast );
	g_AllocManager.printStats();
	g_AllocManager.disable();
		
	printf( "about to exit thread %zd (%zd operations performed)...\n", threadID, iterCount );
}

NOINLINE
void randomTest_FrequentAndInfrequent_RandomUnitSize_UsingNewAndDeleteExp( size_t iterCount, size_t maxFastItemsExp, size_t maxFastItemSizeExp, size_t maxSlowItemsExp, size_t maxSlowItemSizeExp, size_t threadID )
{
	size_t maxFastItems = ((size_t)1) << maxFastItemsExp;
	size_t fastItemIdxMask = maxFastItems - 1;

	size_t maxSlowItems = ((size_t)1) << maxSlowItemsExp;
	size_t slowItemIdxMask = maxSlowItems - 1;

	uint8_t** baseBuffFast = new uint8_t* [maxFastItems];
	uint8_t** baseBuffSlow = new uint8_t* [maxSlowItems];

	memset( baseBuffFast, 0, maxFastItems * sizeof( uint8_t* ) );
	memset( baseBuffSlow, 0, maxSlowItems * sizeof( uint8_t* ) );

	for ( size_t i=0;i<iterCount; ++i )
	{
		{
			size_t randNum = rng();
			size_t idx = randNum & fastItemIdxMask;
			if ( baseBuffFast[idx] )
			{
				delete [] baseBuffFast[idx];
				baseBuffFast[idx] = 0;
			}
			else
			{
				randNum >>= maxFastItemsExp; // let's be economical with randomicity!
				size_t sz = calcSizeWithStatsAdjustment( randNum, maxFastItemSizeExp );
				baseBuffFast[idx] = new uint8_t [sz];
			}
		}
		{
			size_t randNum = rng();
			size_t idx = randNum & slowItemIdxMask;
			if ( baseBuffSlow[idx] )
			{
				delete [] baseBuffSlow[idx];
				baseBuffSlow[idx] = 0;
			}
			else
			{
				randNum >>= maxSlowItemsExp; // let's be economical with randomicity!
				size_t sz = calcSizeWithStatsAdjustment( randNum, maxSlowItemSizeExp );
				baseBuffSlow[idx] = new uint8_t [sz];
			}
		}
	}

	for ( size_t idx=0; idx<maxFastItems; ++idx )
		if ( baseBuffFast[idx] )
			delete [] baseBuffFast[idx];
	for ( size_t idx=0; idx<maxSlowItems; ++idx )
		if ( baseBuffSlow[idx] )
			delete [] baseBuffSlow[idx];
	delete [] baseBuffFast;
	delete [] baseBuffSlow;
	printf( "about to exit thread %zd (%zd operations performed)...\n", threadID, iterCount );
}

NOINLINE
void randomTest_FrequentAndInfrequent_RandomUnitSize_EmptyExp( size_t iterCount, size_t maxFastItemsExp, size_t maxFastItemSizeExp, size_t maxSlowItemsExp, size_t maxSlowItemSizeExp, size_t threadID )
{
	size_t maxFastItems = ((size_t)1) << maxFastItemsExp;
	size_t fastItemIdxMask = maxFastItems - 1;

	size_t maxSlowItems = ((size_t)1) << maxSlowItemsExp;
	size_t slowItemIdxMask = maxSlowItems - 1;

	uint8_t** baseBuffFast = new uint8_t* [maxFastItems];
	uint8_t** baseBuffSlow = new uint8_t* [maxSlowItems];

	memset( baseBuffFast, 0, maxFastItems * sizeof( uint8_t* ) );
	memset( baseBuffSlow, 0, maxSlowItems * sizeof( uint8_t* ) );

	size_t dummyCtr = 0;

	for ( size_t i=0;i<iterCount; ++i )
	{
		{
			size_t randNum = rng();
			size_t idx = randNum & fastItemIdxMask;
			if ( baseBuffFast[idx] )
			{
				dummyCtr += reinterpret_cast<size_t>(baseBuffFast[idx]);
				baseBuffFast[idx] = 0;
			}
			else
			{
				randNum >>= maxFastItemsExp; // let's be economical with randomicity!
				size_t sz = calcSizeWithStatsAdjustment( randNum, maxFastItemSizeExp );
				baseBuffFast[idx] = reinterpret_cast<uint8_t*>(sz); // just imitate the respective operation
			}
		}
		{
			size_t randNum = rng();
			size_t idx = randNum & slowItemIdxMask;
			if ( baseBuffSlow[idx] )
			{
				dummyCtr += reinterpret_cast<size_t>(baseBuffSlow[idx]);
				baseBuffSlow[idx] = 0;
			}
			else
			{
				randNum >>= maxSlowItemsExp; // let's be economical with randomicity!
				size_t sz = calcSizeWithStatsAdjustment( randNum, maxSlowItemSizeExp );
				baseBuffSlow[idx] = reinterpret_cast<uint8_t*>(sz); // just imitate the respective operation
			}
		}
	}

	delete [] baseBuffFast;
	delete [] baseBuffSlow;
	printf( "about to exit thread %zd (%zd operations performed) [ctr = %zd]...\n", threadID, iterCount, dummyCtr );
}


NOINLINE
void randomTest_FrequentAndInfrequent_SkewedBinSelection_RandomUnitSize_UsingPerThreadAllocatorExp( size_t iterCount, size_t maxFastItemsExp, size_t maxFastItemSizeExp, size_t maxSlowItemsExp, size_t maxSlowItemSizeExp, size_t threadID )
{
	const size_t SIZE = 1024 * 1024 * 1024;
	g_AllocManager.initialize( SIZE );
	g_AllocManager.enable();

	size_t maxFastItems = ((size_t)1) << maxFastItemsExp;
	size_t fastItemIdxMask = maxFastItems - 1;

	size_t maxSlowItems = ((size_t)1) << maxSlowItemsExp;
	size_t slowItemIdxMask = maxSlowItems - 1;

	uint8_t** baseBuffFast = reinterpret_cast<uint8_t**>( g_AllocManager.allocate( maxFastItems * sizeof(uint8_t*) ) );
	uint8_t** baseBuffSlow = reinterpret_cast<uint8_t**>( g_AllocManager.allocate( maxSlowItems * sizeof(uint8_t*) ) );

	memset( baseBuffFast, 0, maxFastItems * sizeof( uint8_t* ) );
	memset( baseBuffSlow, 0, maxSlowItems * sizeof( uint8_t* ) );

	for ( size_t i=0;i<iterCount; ++i )
	{
		{
			size_t randNum = rng();
//			size_t idx = randNum & fastItemIdxMask;
			size_t idx = calcSizeWithStatsAdjustment( randNum, maxFastItemsExp ) - 1;
			if ( baseBuffFast[idx] )
			{
				g_AllocManager.deallocate( baseBuffFast[idx] );
				baseBuffFast[idx] = 0;
			}
			else
			{
//				randNum >>= maxFastItemsExp; // let's be economical with randomicity!
				randNum = rng();
				size_t sz = calcSizeWithStatsAdjustment( randNum, maxFastItemSizeExp );
				baseBuffFast[idx] = reinterpret_cast<uint8_t*>( g_AllocManager.allocate( sz ) );
			}
		}
		{
			size_t randNum = rng();
//			size_t idx = randNum & slowItemIdxMask;
			size_t idx = calcSizeWithStatsAdjustment( randNum, maxSlowItemsExp ) - 1;
			if ( baseBuffSlow[idx] )
			{
				g_AllocManager.deallocate( baseBuffSlow[idx] );
				baseBuffSlow[idx] = 0;
			}
			else
			{
//				randNum >>= maxSlowItemsExp; // let's be economical with randomicity!
				randNum = rng();
				size_t sz = calcSizeWithStatsAdjustment( randNum, maxSlowItemSizeExp );
				baseBuffSlow[idx] = reinterpret_cast<uint8_t*>( g_AllocManager.allocate( sz ) );
			}
		}
	}

	for ( size_t idx=0; idx<maxFastItems; ++idx )
		if ( baseBuffFast[idx] )
			g_AllocManager.deallocate( baseBuffFast[idx] );
	for ( size_t idx=0; idx<maxSlowItems; ++idx )
		if ( baseBuffSlow[idx] )
			g_AllocManager.deallocate( baseBuffSlow[idx] );
	g_AllocManager.deallocate( baseBuffSlow );
	g_AllocManager.deallocate( baseBuffFast );
	g_AllocManager.printStats();
	g_AllocManager.disable();
		
	printf( "about to exit thread %zd (%zd operations performed)...\n", threadID, iterCount );
}

NOINLINE
void randomTest_FrequentAndInfrequent_SkewedBinSelection_RandomUnitSize_UsingNewAndDeleteExp( size_t iterCount, size_t maxFastItemsExp, size_t maxFastItemSizeExp, size_t maxSlowItemsExp, size_t maxSlowItemSizeExp, size_t threadID )
{
	size_t maxFastItems = ((size_t)1) << maxFastItemsExp;
	size_t fastItemIdxMask = maxFastItems - 1;

	size_t maxSlowItems = ((size_t)1) << maxSlowItemsExp;
	size_t slowItemIdxMask = maxSlowItems - 1;

	uint8_t** baseBuffFast = new uint8_t* [maxFastItems];
	uint8_t** baseBuffSlow = new uint8_t* [maxSlowItems];

	memset( baseBuffFast, 0, maxFastItems * sizeof( uint8_t* ) );
	memset( baseBuffSlow, 0, maxSlowItems * sizeof( uint8_t* ) );

	for ( size_t i=0;i<iterCount; ++i )
	{
		{
			size_t randNum = rng();
//			size_t idx = randNum & fastItemIdxMask;
			size_t idx = calcSizeWithStatsAdjustment( randNum, maxFastItemsExp ) - 1;
			if ( baseBuffFast[idx] )
			{
				delete [] baseBuffFast[idx];
				baseBuffFast[idx] = 0;
			}
			else
			{
//				randNum >>= maxFastItemsExp; // let's be economical with randomicity!
				randNum = rng();
				size_t sz = calcSizeWithStatsAdjustment( randNum, maxFastItemSizeExp );
				baseBuffFast[idx] = new uint8_t [sz];
			}
		}
		{
			size_t randNum = rng();
//			size_t idx = randNum & slowItemIdxMask;
			size_t idx = calcSizeWithStatsAdjustment( randNum, maxSlowItemsExp ) - 1;
			if ( baseBuffSlow[idx] )
			{
				delete [] baseBuffSlow[idx];
				baseBuffSlow[idx] = 0;
			}
			else
			{
//				randNum >>= maxSlowItemsExp; // let's be economical with randomicity!
				randNum = rng();
				size_t sz = calcSizeWithStatsAdjustment( randNum, maxSlowItemSizeExp );
				baseBuffSlow[idx] = new uint8_t [sz];
			}
		}
	}

	for ( size_t idx=0; idx<maxFastItems; ++idx )
		if ( baseBuffFast[idx] )
			delete [] baseBuffFast[idx];
	for ( size_t idx=0; idx<maxSlowItems; ++idx )
		if ( baseBuffSlow[idx] )
			delete [] baseBuffSlow[idx];
	delete [] baseBuffFast;
	delete [] baseBuffSlow;
	printf( "about to exit thread %zd (%zd operations performed)...\n", threadID, iterCount );
}

NOINLINE
void randomTest_FrequentAndInfrequent_SkewedBinSelection_RandomUnitSize_EmptyExp( size_t iterCount, size_t maxFastItemsExp, size_t maxFastItemSizeExp, size_t maxSlowItemsExp, size_t maxSlowItemSizeExp, size_t threadID )
{
	size_t maxFastItems = ((size_t)1) << maxFastItemsExp;
	size_t fastItemIdxMask = maxFastItems - 1;

	size_t maxSlowItems = ((size_t)1) << maxSlowItemsExp;
	size_t slowItemIdxMask = maxSlowItems - 1;

	uint8_t** baseBuffFast = new uint8_t* [maxFastItems];
	uint8_t** baseBuffSlow = new uint8_t* [maxSlowItems];

	memset( baseBuffFast, 0, maxFastItems * sizeof( uint8_t* ) );
	memset( baseBuffSlow, 0, maxSlowItems * sizeof( uint8_t* ) );

	size_t dummyCtr = 0;

	for ( size_t i=0;i<iterCount; ++i )
	{
		{
			size_t randNum = rng();
//			size_t idx = randNum & fastItemIdxMask;
			size_t idx = calcSizeWithStatsAdjustment( randNum, maxFastItemsExp ) - 1;
			if ( baseBuffFast[idx] )
			{
				dummyCtr += reinterpret_cast<size_t>(baseBuffFast[idx]);
				baseBuffFast[idx] = 0;
			}
			else
			{
//				randNum >>= maxFastItemsExp; // let's be economical with randomicity!
				randNum = rng();
				size_t sz = calcSizeWithStatsAdjustment( randNum, maxFastItemSizeExp );
				baseBuffFast[idx] = reinterpret_cast<uint8_t*>(sz); // just imitate the respective operation
			}
		}
		{
			size_t randNum = rng();
//			size_t idx = randNum & slowItemIdxMask;
			size_t idx = calcSizeWithStatsAdjustment( randNum, maxSlowItemsExp ) - 1;
			if ( baseBuffSlow[idx] )
			{
				dummyCtr += reinterpret_cast<size_t>(baseBuffSlow[idx]);
				baseBuffSlow[idx] = 0;
			}
			else
			{
//				randNum >>= maxSlowItemsExp; // let's be economical with randomicity!
				randNum = rng();
				size_t sz = calcSizeWithStatsAdjustment( randNum, maxSlowItemSizeExp );
				baseBuffSlow[idx] = reinterpret_cast<uint8_t*>(sz); // just imitate the respective operation
			}
		}
	}


	for ( size_t idx=0; idx<maxFastItems; ++idx )
		if ( baseBuffFast[idx] )
			++dummyCtr;
	for ( size_t idx=0; idx<maxSlowItems; ++idx )
		if ( baseBuffSlow[idx] )
			++dummyCtr;
	delete [] baseBuffFast;
	delete [] baseBuffSlow;
	printf( "about to exit thread %zd (%zd operations performed) [ctr = %zd]...\n", threadID, iterCount, dummyCtr );
}


NOINLINE
void randomTest_FrequentAndInfrequentWithAccess_RandomUnitSize_UsingPerThreadAllocatorExp( size_t iterCount, size_t maxFastItemsExp, size_t maxFastItemSizeExp, size_t maxSlowItemsExp, size_t maxSlowItemSizeExp, size_t memAccessPerRound, size_t threadID )
{
	const size_t SIZE = 1024 * 1024 * 1024;
	g_AllocManager.initialize( SIZE );
	g_AllocManager.enable();

	size_t maxFastItems = ((size_t)1) << maxFastItemsExp;
	size_t fastItemIdxMask = maxFastItems - 1;

	size_t maxSlowItems = ((size_t)1) << maxSlowItemsExp;
	size_t slowItemIdxMask = maxSlowItems - 1;

	uint8_t** baseBuffFast = reinterpret_cast<uint8_t**>( g_AllocManager.allocate( maxFastItems * sizeof(uint8_t*) ) );
	uint8_t** baseBuffSlow = reinterpret_cast<uint8_t**>( g_AllocManager.allocate( maxSlowItems * sizeof(uint8_t*) ) );

	memset( baseBuffFast, 0, maxFastItems * sizeof( uint8_t* ) );
	memset( baseBuffSlow, 0, maxSlowItems * sizeof( uint8_t* ) );

	size_t dummyCtr = 0;

	// 1: setup
	for ( size_t i=0;i<maxFastItems; ++i )//memAccessPerRound
	{
		size_t randNum = rng();
		size_t sz = calcSizeWithStatsAdjustment( randNum, maxFastItemSizeExp );
		baseBuffFast[i] = reinterpret_cast<uint8_t*>( g_AllocManager.allocate( sz ) );
	}
	for ( size_t i=0;i<maxSlowItems; ++i )//memAccessPerRound
	{
		size_t randNum = rng();
		size_t sz = calcSizeWithStatsAdjustment( randNum, maxSlowItemSizeExp );
		baseBuffSlow[i] = reinterpret_cast<uint8_t*>( g_AllocManager.allocate( sz ) );
	}
	// 2: test
	for ( size_t i=0;i<iterCount; ++i )//memAccessPerRound
	{
		{
			size_t randNum = rng();
			size_t idx = randNum & fastItemIdxMask;
			randNum >>= maxFastItemsExp; // let's be economical with randomicity!
			size_t sz = calcSizeWithStatsAdjustment( randNum, maxFastItemSizeExp );
			g_AllocManager.deallocate( baseBuffFast[idx] );
			baseBuffFast[idx] = reinterpret_cast<uint8_t*>( g_AllocManager.allocate( sz ) );
			uint32_t randNum1 = (uint32_t)(rng());
			for ( size_t j=0; j<memAccessPerRound; ++j )
			{
				dummyCtr += *(baseBuffFast[randNum1 & fastItemIdxMask]);
				randNum1 = xorshift32( randNum1 );
			}
		}
		{
			size_t randNum = rng();
			size_t idx = randNum & slowItemIdxMask;
			randNum >>= maxSlowItemsExp; // let's be economical with randomicity!
			size_t sz = calcSizeWithStatsAdjustment( randNum, maxSlowItemSizeExp );
			g_AllocManager.deallocate( baseBuffSlow[idx] );
			baseBuffSlow[idx] = reinterpret_cast<uint8_t*>( g_AllocManager.allocate( sz ) );
			uint32_t randNum1 = (uint32_t)(rng());
			for ( size_t j=0; j<memAccessPerRound; ++j )
			{
				dummyCtr += *(baseBuffSlow[randNum1 & slowItemIdxMask]);
				randNum1 = xorshift32( randNum1 );
			}
		}
	}
	// 3: cleanup
	for ( size_t i=0;i<maxFastItems; ++i )//memAccessPerRound
		g_AllocManager.deallocate( baseBuffFast[i] );
	for ( size_t i=0;i<maxSlowItems; ++i )//memAccessPerRound
		g_AllocManager.deallocate( baseBuffSlow[i] );

	g_AllocManager.deallocate( baseBuffSlow );
	g_AllocManager.deallocate( baseBuffFast );
	g_AllocManager.printStats();
	g_AllocManager.disable();
		
	printf( "about to exit thread %zd (%zd operations performed) [ctr = %zd]...\n", threadID, iterCount, dummyCtr );
}

NOINLINE
void randomTest_FrequentAndInfrequentWithAccess_RandomUnitSize_UsingNewAndDeleteExp( size_t iterCount, size_t maxFastItemsExp, size_t maxFastItemSizeExp, size_t maxSlowItemsExp, size_t maxSlowItemSizeExp, size_t memAccessPerRound, size_t threadID )
{
	size_t maxFastItems = ((size_t)1) << maxFastItemsExp;
	size_t fastItemIdxMask = maxFastItems - 1;

	size_t maxSlowItems = ((size_t)1) << maxSlowItemsExp;
	size_t slowItemIdxMask = maxSlowItems - 1;

	uint8_t** baseBuffFast = new uint8_t* [maxFastItems];
	uint8_t** baseBuffSlow = new uint8_t* [maxSlowItems];

	memset( baseBuffFast, 0, maxFastItems * sizeof( uint8_t* ) );
	memset( baseBuffSlow, 0, maxSlowItems * sizeof( uint8_t* ) );

	size_t dummyCtr = 0;

	// 1: setup
	for ( size_t i=0;i<maxFastItems; ++i )//memAccessPerRound
	{
		size_t randNum = rng();
		size_t sz = calcSizeWithStatsAdjustment( randNum, maxFastItemSizeExp );
		baseBuffFast[i] = new uint8_t [sz];
	}
	for ( size_t i=0;i<maxSlowItems; ++i )//memAccessPerRound
	{
		size_t randNum = rng();
		size_t sz = calcSizeWithStatsAdjustment( randNum, maxSlowItemSizeExp );
		baseBuffSlow[i] = new uint8_t [sz];
	}
	// 2: test
	for ( size_t i=0;i<iterCount; ++i )//memAccessPerRound
	{
		{
			size_t randNum = rng();
			size_t idx = randNum & fastItemIdxMask;
			randNum >>= maxFastItemsExp; // let's be economical with randomicity!
			size_t sz = calcSizeWithStatsAdjustment( randNum, maxFastItemSizeExp );
			delete [] baseBuffFast[idx];
			baseBuffFast[idx] = new uint8_t [sz];
			uint32_t randNum1 = (uint32_t)(rng());
			for ( size_t j=0; j<memAccessPerRound; ++j )
			{
				dummyCtr += *(baseBuffFast[randNum1 & fastItemIdxMask]);
				randNum1 = xorshift32( randNum1 );
			}
		}
		{
			size_t randNum = rng();
			size_t idx = randNum & slowItemIdxMask;
			randNum >>= maxSlowItemsExp; // let's be economical with randomicity!
			size_t sz = calcSizeWithStatsAdjustment( randNum, maxSlowItemSizeExp );
			delete [] baseBuffSlow[idx];
			baseBuffSlow[idx] = new uint8_t [sz];
			uint32_t randNum1 = (uint32_t)(rng());
			for ( size_t j=0; j<memAccessPerRound; ++j )
			{
				dummyCtr += *(baseBuffSlow[randNum1 & slowItemIdxMask]);
				randNum1 = xorshift32( randNum1 );
			}
		}
	}
	// 3: cleanup
	for ( size_t i=0;i<maxFastItems; ++i )//memAccessPerRound
		delete [] baseBuffFast[i];
	for ( size_t i=0;i<maxSlowItems; ++i )//memAccessPerRound
		delete [] baseBuffSlow[i];

	delete [] baseBuffFast;
	delete [] baseBuffSlow;
	printf( "about to exit thread %zd (%zd operations performed) [ctr = %zd]...\n", threadID, iterCount, dummyCtr );
}

NOINLINE
void randomTest_FrequentAndInfrequentWithAccess_RandomUnitSize_EmptyExp( size_t iterCount, size_t maxFastItemsExp, size_t maxFastItemSizeExp, size_t maxSlowItemsExp, size_t maxSlowItemSizeExp, size_t memAccessPerRound, size_t threadID )
{
	size_t maxFastItems = ((size_t)1) << maxFastItemsExp;
	size_t fastItemIdxMask = maxFastItems - 1;

	size_t maxSlowItems = ((size_t)1) << maxSlowItemsExp;
	size_t slowItemIdxMask = maxSlowItems - 1;

	uint8_t** baseBuffFast = new uint8_t* [maxFastItems];
	uint8_t** baseBuffSlow = new uint8_t* [maxSlowItems];

	memset( baseBuffFast, 0, maxFastItems * sizeof( uint8_t* ) );
	memset( baseBuffSlow, 0, maxSlowItems * sizeof( uint8_t* ) );

	size_t dummyCtr = 0;

	// 1: setup
	for ( size_t i=0;i<maxFastItems; ++i )//memAccessPerRound
	{
		size_t randNum = rng();
		size_t sz = calcSizeWithStatsAdjustment( randNum, maxFastItemSizeExp );
		baseBuffFast[i] = reinterpret_cast<uint8_t*>(sz); // just imitate the respective operation
	}
	for ( size_t i=0;i<maxSlowItems; ++i )//memAccessPerRound
	{
		size_t randNum = rng();
		size_t sz = calcSizeWithStatsAdjustment( randNum, maxSlowItemSizeExp );
		baseBuffSlow[i] = reinterpret_cast<uint8_t*>(sz); // just imitate the respective operation
	}
	// 2: test
	for ( size_t i=0;i<iterCount; ++i )//
	{
		{
			size_t randNum = rng();
			size_t idx = randNum & fastItemIdxMask;
			randNum >>= maxFastItemsExp; // let's be economical with randomicity!
			size_t sz = calcSizeWithStatsAdjustment( randNum, maxFastItemSizeExp );
			dummyCtr += reinterpret_cast<size_t>(baseBuffFast[idx]);
			baseBuffFast[idx] = reinterpret_cast<uint8_t*>(sz); // just imitate the respective operation
			uint32_t randNum1 = (uint32_t)(rng());
			for ( size_t j=0; j<memAccessPerRound; ++j )
			{
				dummyCtr += reinterpret_cast<size_t>(baseBuffFast[randNum1 & fastItemIdxMask]);
				randNum1 = xorshift32( randNum1 );
			}
		}
		{
			size_t randNum = rng();
			size_t idx = randNum & slowItemIdxMask;
			randNum >>= maxSlowItemsExp; // let's be economical with randomicity!
			size_t sz = calcSizeWithStatsAdjustment( randNum, maxSlowItemSizeExp );
			dummyCtr += reinterpret_cast<size_t>(baseBuffSlow[idx]);
			baseBuffSlow[idx] = reinterpret_cast<uint8_t*>(sz); // just imitate the respective operation
			uint32_t randNum1 = (uint32_t)(rng());
			for ( size_t j=0; j<memAccessPerRound; ++j )
			{
				dummyCtr += reinterpret_cast<size_t>(baseBuffSlow[randNum1 & slowItemIdxMask]);
				randNum1 = xorshift32( randNum1 );
			}
		}
	}
	// 3: cleanup
	for ( size_t i=0;i<maxFastItems; ++i )//memAccessPerRound
		dummyCtr += reinterpret_cast<size_t>(baseBuffFast[i]);
	for ( size_t i=0;i<maxSlowItems; ++i )//memAccessPerRound
		dummyCtr += reinterpret_cast<size_t>(baseBuffSlow[i]);

	delete [] baseBuffFast;
	delete [] baseBuffSlow;
	printf( "about to exit thread %zd (%zd operations performed) [ctr = %zd]...\n", threadID, iterCount, dummyCtr );
}


NOINLINE
void randomTest_FrequentAndInfrequentWithAccess_SkewedBinSelection_RandomUnitSize_UsingPerThreadAllocatorExp( size_t iterCount, size_t maxFastItemsExp, size_t maxFastItemSizeExp, size_t maxSlowItemsExp, size_t maxSlowItemSizeExp, size_t memAccessPerRound, size_t threadID )
{
	const size_t SIZE = 1024 * 1024 * 1024;
	g_AllocManager.initialize( SIZE );
	g_AllocManager.enable();

	size_t maxFastItems = ((size_t)1) << maxFastItemsExp;
	size_t fastItemIdxMask = maxFastItems - 1;

	size_t maxSlowItems = ((size_t)1) << maxSlowItemsExp;
	size_t slowItemIdxMask = maxSlowItems - 1;

	uint8_t** baseBuffFast = reinterpret_cast<uint8_t**>( g_AllocManager.allocate( maxFastItems * sizeof(uint8_t*) ) );
	uint8_t** baseBuffSlow = reinterpret_cast<uint8_t**>( g_AllocManager.allocate( maxSlowItems * sizeof(uint8_t*) ) );

	memset( baseBuffFast, 0, maxFastItems * sizeof( uint8_t* ) );
	memset( baseBuffSlow, 0, maxSlowItems * sizeof( uint8_t* ) );

	memAccessPerRound *= 2; // as each bin may be just empty with probability 1/2

	size_t dummyCtr = 0;

	for ( size_t i=0;i<iterCount; ++i )
	{
		{
			size_t randNum = rng();
//			size_t idx = randNum & fastItemIdxMask;
			size_t idx = calcSizeWithStatsAdjustment( randNum, maxFastItemsExp ) - 1;
			if ( baseBuffFast[idx] )
			{
				g_AllocManager.deallocate( baseBuffFast[idx] );
				baseBuffFast[idx] = 0;
			}
			else
			{
//				randNum >>= maxFastItemsExp; // let's be economical with randomicity!
				randNum = rng();
				size_t sz = calcSizeWithStatsAdjustment( randNum, maxFastItemSizeExp );
				baseBuffFast[idx] = reinterpret_cast<uint8_t*>( g_AllocManager.allocate( sz ) );
			}
			uint32_t randNum1 = (uint32_t)(rng());
			for ( size_t j=0; j<memAccessPerRound; ++j )
			{
				if ( baseBuffFast[randNum1 & fastItemIdxMask] != nullptr )
					dummyCtr += *(baseBuffFast[randNum1 & fastItemIdxMask]);
				randNum1 = xorshift32( randNum1 );
			}
		}
		{
			size_t randNum = rng();
//			size_t idx = randNum & slowItemIdxMask;
			size_t idx = calcSizeWithStatsAdjustment( randNum, maxSlowItemsExp ) - 1;
			if ( baseBuffSlow[idx] )
			{
				g_AllocManager.deallocate( baseBuffSlow[idx] );
				baseBuffSlow[idx] = 0;
			}
			else
			{
//				randNum >>= maxSlowItemsExp; // let's be economical with randomicity!
				randNum = rng();
				size_t sz = calcSizeWithStatsAdjustment( randNum, maxSlowItemSizeExp );
				baseBuffSlow[idx] = reinterpret_cast<uint8_t*>( g_AllocManager.allocate( sz ) );
			}
			uint32_t randNum1 = (uint32_t)(rng());
			for ( size_t j=0; j<memAccessPerRound; ++j )
			{
				if ( baseBuffSlow[randNum1 & slowItemIdxMask] != nullptr )
					dummyCtr += *(baseBuffSlow[randNum1 & slowItemIdxMask]);
				randNum1 = xorshift32( randNum1 );
			}
		}
	}

	for ( size_t idx=0; idx<maxFastItems; ++idx )
		if ( baseBuffFast[idx] )
			g_AllocManager.deallocate( baseBuffFast[idx] );
	for ( size_t idx=0; idx<maxSlowItems; ++idx )
		if ( baseBuffSlow[idx] )
			g_AllocManager.deallocate( baseBuffSlow[idx] );
	g_AllocManager.deallocate( baseBuffSlow );
	g_AllocManager.deallocate( baseBuffFast );
	g_AllocManager.printStats();
	g_AllocManager.disable();
		
	printf( "about to exit thread %zd (%zd operations performed) [ctr = %zd]...\n", threadID, iterCount, dummyCtr );
}

NOINLINE
void randomTest_FrequentAndInfrequentWithAccess_SkewedBinSelection_RandomUnitSize_UsingNewAndDeleteExp( size_t iterCount, size_t maxFastItemsExp, size_t maxFastItemSizeExp, size_t maxSlowItemsExp, size_t maxSlowItemSizeExp, size_t memAccessPerRound, size_t threadID )
{
	size_t maxFastItems = ((size_t)1) << maxFastItemsExp;
	size_t fastItemIdxMask = maxFastItems - 1;

	size_t maxSlowItems = ((size_t)1) << maxSlowItemsExp;
	size_t slowItemIdxMask = maxSlowItems - 1;

	uint8_t** baseBuffFast = new uint8_t* [maxFastItems];
	uint8_t** baseBuffSlow = new uint8_t* [maxSlowItems];

	memset( baseBuffFast, 0, maxFastItems * sizeof( uint8_t* ) );
	memset( baseBuffSlow, 0, maxSlowItems * sizeof( uint8_t* ) );

	memAccessPerRound *= 2; // as each bin may be just empty with probability 1/2

	size_t dummyCtr = 0;

	for ( size_t i=0;i<iterCount; ++i )
	{
		{
			size_t randNum = rng();
//			size_t idx = randNum & fastItemIdxMask;
			size_t idx = calcSizeWithStatsAdjustment( randNum, maxFastItemsExp ) - 1;
			if ( baseBuffFast[idx] )
			{
				delete [] baseBuffFast[idx];
				baseBuffFast[idx] = 0;
			}
			else
			{
//				randNum >>= maxFastItemsExp; // let's be economical with randomicity!
				randNum = rng();
				size_t sz = calcSizeWithStatsAdjustment( randNum, maxFastItemSizeExp );
				baseBuffFast[idx] = new uint8_t [sz];
			}
			uint32_t randNum1 = (uint32_t)(rng());
			for ( size_t j=0; j<memAccessPerRound; ++j )
			{
				if ( baseBuffFast[randNum1 & fastItemIdxMask] != nullptr )
					dummyCtr += *(baseBuffFast[randNum1 & fastItemIdxMask]);
				randNum1 = xorshift32( randNum1 );
			}
		}
		{
			size_t randNum = rng();
//			size_t idx = randNum & slowItemIdxMask;
			size_t idx = calcSizeWithStatsAdjustment( randNum, maxSlowItemsExp ) - 1;
			if ( baseBuffSlow[idx] )
			{
				delete [] baseBuffSlow[idx];
				baseBuffSlow[idx] = 0;
			}
			else
			{
//				randNum >>= maxSlowItemsExp; // let's be economical with randomicity!
				randNum = rng();
				size_t sz = calcSizeWithStatsAdjustment( randNum, maxSlowItemSizeExp );
				baseBuffSlow[idx] = new uint8_t [sz];
			}
			uint32_t randNum1 = (uint32_t)(rng());
			for ( size_t j=0; j<memAccessPerRound; ++j )
			{
				if ( baseBuffSlow[randNum1 & slowItemIdxMask] != nullptr )
					dummyCtr += *(baseBuffSlow[randNum1 & slowItemIdxMask]);
				randNum1 = xorshift32( randNum1 );
			}
		}
	}

	for ( size_t idx=0; idx<maxFastItems; ++idx )
		if ( baseBuffFast[idx] )
			delete [] baseBuffFast[idx];
	for ( size_t idx=0; idx<maxSlowItems; ++idx )
		if ( baseBuffSlow[idx] )
			delete [] baseBuffSlow[idx];
	delete [] baseBuffFast;
	delete [] baseBuffSlow;
	printf( "about to exit thread %zd (%zd operations performed) [ctr = %zd]...\n", threadID, iterCount, dummyCtr );
}

NOINLINE
void randomTest_FrequentAndInfrequentWithAccess_SkewedBinSelection_RandomUnitSize_EmptyExp( size_t iterCount, size_t maxFastItemsExp, size_t maxFastItemSizeExp, size_t maxSlowItemsExp, size_t maxSlowItemSizeExp, size_t memAccessPerRound, size_t threadID )
{
	size_t maxFastItems = ((size_t)1) << maxFastItemsExp;
	size_t fastItemIdxMask = maxFastItems - 1;

	size_t maxSlowItems = ((size_t)1) << maxSlowItemsExp;
	size_t slowItemIdxMask = maxSlowItems - 1;

	uint8_t** baseBuffFast = new uint8_t* [maxFastItems];
	uint8_t** baseBuffSlow = new uint8_t* [maxSlowItems];

	memset( baseBuffFast, 0, maxFastItems * sizeof( uint8_t* ) );
	memset( baseBuffSlow, 0, maxSlowItems * sizeof( uint8_t* ) );

	memAccessPerRound *= 2; // as each bin may be just empty with probability 1/2

	size_t dummyCtr = 0;

	for ( size_t i=0;i<iterCount; ++i )
	{
		{
			size_t randNum = rng();
//			size_t idx = randNum & fastItemIdxMask;
			size_t idx = calcSizeWithStatsAdjustment( randNum, maxFastItemsExp ) - 1;
			if ( baseBuffFast[idx] )
			{
				dummyCtr += reinterpret_cast<size_t>(baseBuffFast[idx]);
				baseBuffFast[idx] = 0;
			}
			else
			{
//				randNum >>= maxFastItemsExp; // let's be economical with randomicity!
				randNum = rng();
				size_t sz = calcSizeWithStatsAdjustment( randNum, maxFastItemSizeExp );
				baseBuffFast[idx] = reinterpret_cast<uint8_t*>(sz); // just imitate the respective operation
			}
			uint32_t randNum1 = (uint32_t)(rng());
			for ( size_t j=0; j<memAccessPerRound; ++j )
			{
				if ( baseBuffFast[randNum1 & fastItemIdxMask] != nullptr )
					dummyCtr += reinterpret_cast<size_t>(baseBuffFast[randNum1 & fastItemIdxMask]);
				randNum1 = xorshift32( randNum1 );
			}
		}
		{
			size_t randNum = rng();
//			size_t idx = randNum & slowItemIdxMask;
			size_t idx = calcSizeWithStatsAdjustment( randNum, maxSlowItemsExp ) - 1;
			if ( baseBuffSlow[idx] )
			{
				dummyCtr += reinterpret_cast<size_t>(baseBuffSlow[idx]);
				baseBuffSlow[idx] = 0;
			}
			else
			{
//				randNum >>= maxSlowItemsExp; // let's be economical with randomicity!
				randNum = rng();
				size_t sz = calcSizeWithStatsAdjustment( randNum, maxSlowItemSizeExp );
				baseBuffSlow[idx] = reinterpret_cast<uint8_t*>(sz); // just imitate the respective operation
			}
			uint32_t randNum1 = (uint32_t)(rng());
			for ( size_t j=0; j<memAccessPerRound; ++j )
			{
				if ( baseBuffSlow[randNum1 & slowItemIdxMask] != nullptr )
					dummyCtr += reinterpret_cast<size_t>(baseBuffSlow[randNum1 & slowItemIdxMask]);
				randNum1 = xorshift32( randNum1 );
			}
		}
	}

	for ( size_t idx=0; idx<maxFastItems; ++idx )
		if ( baseBuffFast[idx] )
			++dummyCtr;
	for ( size_t idx=0; idx<maxSlowItems; ++idx )
		if ( baseBuffSlow[idx] )
			++dummyCtr;
	delete [] baseBuffFast;
	delete [] baseBuffSlow;
	printf( "about to exit thread %zd (%zd operations performed) [ctr = %zd]...\n", threadID, iterCount, dummyCtr );
}


enum { USE_PER_THREAD_ALLOCATOR, USE_NEW_DELETE, USE_EMPTY_TEST };
enum { USE_RANDOMPOS_FIXEDSIZE, USE_RANDOMPOS_FULLMEMACCESS_FIXEDSIZE, USE_RANDOMPOS_RANDOMSIZE, USE_DEALLOCALLOCLEASTRECENTLYUSED_RANDOMUNITSIZE, USE_DEALLOCALLOCLEASTRECENTLYUSED_SAMEUNITSIZE, 
	USE_FREQUENTANDINFREQUENT_RANDOMUNITSIZE, USE_FREQUENTANDINFREQUENT_SKEWEDBINSELECTION_RANDOMUNITSIZE, USE_FREQUENTANDINFREQUENTWITHACCESS_RANDOMUNITSIZE, USE_FREQUENTANDINFREQUENTWITHACCESS_SKEWEDBINSELECTION_RANDOMUNITSIZE };

struct TestStartupParams
{
	size_t threadID;
	size_t calcMod ;
	size_t maxItems;
	size_t maxItemSize;
	size_t maxItems2;
	size_t maxItemSize2;
	size_t memReadCnt;
	size_t iterCount;
	size_t usePerThreadAllocator;
};

void* runRandomTest( void* params )
{
	assert( params != nullptr );
	TestStartupParams* testParams = reinterpret_cast<TestStartupParams*>( params );
	switch ( testParams->calcMod )
	{
		case USE_RANDOMPOS_FIXEDSIZE:
		{
			switch ( testParams->usePerThreadAllocator )
			{
				case USE_PER_THREAD_ALLOCATOR:
					printf( "    running thread %zd with randomPos_FixedSize_UsingPerThreadAllocator() ...\n", testParams->threadID );
					randomPos_FixedSize_UsingPerThreadAllocatorExp( testParams->iterCount, testParams->maxItems, testParams->maxItemSize, testParams->threadID );
					break;
				case USE_NEW_DELETE:
					printf( "    running thread %zd with randomPos_FixedSize_UsingNewAndDelete() ...\n", testParams->threadID );
					randomPos_FixedSize_UsingNewAndDeleteExp( testParams->iterCount, testParams->maxItems, testParams->maxItemSize, testParams->threadID );
					break;
				case USE_EMPTY_TEST:
					printf( "    running thread %zd with randomPos_FixedSize_Empty() ...\n", testParams->threadID );
					randomPos_FixedSize_EmptyExp( testParams->iterCount, testParams->maxItems, testParams->maxItemSize, testParams->threadID );
					break;
				default:
					assert( false );
			}
			break;
		}
		case USE_RANDOMPOS_FULLMEMACCESS_FIXEDSIZE:
		{
			switch ( testParams->usePerThreadAllocator )
			{
				case USE_PER_THREAD_ALLOCATOR:
					printf( "    running thread %zd with randomPos_FixedSize_FullMemAccess_UsingPerThreadAllocator() ...\n", testParams->threadID );
					randomPos_FixedSize_FullMemAccess_UsingPerThreadAllocatorExp( testParams->iterCount, testParams->maxItems, testParams->maxItemSize, testParams->threadID );
					break;
				case USE_NEW_DELETE:
					printf( "    running thread %zd with randomPos_FixedSize_FullMemAccess_UsingNewAndDelete() ...\n", testParams->threadID );
					randomPos_FixedSize_FullMemAccess_UsingNewAndDeleteExp( testParams->iterCount, testParams->maxItems, testParams->maxItemSize, testParams->threadID );
					break;
				case USE_EMPTY_TEST:
					printf( "    running thread %zd with randomPos_FixedSize_FullMemAccess_Empty() ...\n", testParams->threadID );
					randomPos_FixedSize_FullMemAccess_EmptyExp( testParams->iterCount, testParams->maxItems, testParams->maxItemSize, testParams->threadID );
					break;
				default:
					assert( false );
			}
			break;
		}
		case USE_RANDOMPOS_RANDOMSIZE:
		{
			switch ( testParams->usePerThreadAllocator )
			{
				case USE_PER_THREAD_ALLOCATOR:
					printf( "    running thread %zd with randomTestUsingPerThreadAllocator() ...\n", testParams->threadID );
					randomTestUsingPerThreadAllocatorExp( testParams->iterCount, testParams->maxItems, testParams->maxItemSize, testParams->threadID );
					break;
				case USE_NEW_DELETE:
					printf( "    running thread %zd with randomTestUsingNewAndDelete() ...\n", testParams->threadID );
					randomTestUsingNewAndDeleteExp( testParams->iterCount, testParams->maxItems, testParams->maxItemSize, testParams->threadID );
					break;
				case USE_EMPTY_TEST:
					printf( "    running thread %zd with randomTestEmpty() ...\n", testParams->threadID );
					randomTestEmptyExp( testParams->iterCount, testParams->maxItems, testParams->maxItemSize, testParams->threadID );
					break;
				default:
					assert( false );
			}
			break;
		}
		case USE_DEALLOCALLOCLEASTRECENTLYUSED_RANDOMUNITSIZE:
		{
			switch ( testParams->usePerThreadAllocator )
			{
				case USE_PER_THREAD_ALLOCATOR:
					printf( "    running thread %zd with randomTestUsingPerThreadAllocator() ...\n", testParams->threadID );
					randomTest_DeallocAllocLeastRecentlyUsed_RandomUnitSize_UsingPerThreadAllocatorExp( testParams->iterCount, testParams->maxItems, testParams->maxItemSize, testParams->threadID );
					break;
				case USE_NEW_DELETE:
					printf( "    running thread %zd with randomTestUsingNewAndDelete() ...\n", testParams->threadID );
					randomTest_DeallocAllocLeastRecentlyUsed_RandomUnitSize_UsingNewAndDeleteExp( testParams->iterCount, testParams->maxItems, testParams->maxItemSize, testParams->threadID );
					break;
				case USE_EMPTY_TEST:
					printf( "    running thread %zd with randomTestEmpty() ...\n", testParams->threadID );
					randomTest_DeallocAllocLeastRecentlyUsed_RandomUnitSize_EmptyExp( testParams->iterCount, testParams->maxItems, testParams->maxItemSize, testParams->threadID );
					break;
				default:
					assert( false );
			}
			break;
		}
		case USE_DEALLOCALLOCLEASTRECENTLYUSED_SAMEUNITSIZE:
		{
			switch ( testParams->usePerThreadAllocator )
			{
				case USE_PER_THREAD_ALLOCATOR:
					printf( "    running thread %zd with randomTestUsingPerThreadAllocator() ...\n", testParams->threadID );
					randomTest_DeallocAllocLeastRecentlyUsed_SameUnitSize_UsingPerThreadAllocatorExp( testParams->iterCount, testParams->maxItems, testParams->maxItemSize, testParams->threadID );
					break;
				case USE_NEW_DELETE:
					printf( "    running thread %zd with randomTestUsingNewAndDelete() ...\n", testParams->threadID );
					randomTest_DeallocAllocLeastRecentlyUsed_SameUnitSize_UsingNewAndDeleteExp( testParams->iterCount, testParams->maxItems, testParams->maxItemSize, testParams->threadID );
					break;
				case USE_EMPTY_TEST:
					printf( "    running thread %zd with randomTestEmpty() ...\n", testParams->threadID );
					randomTest_DeallocAllocLeastRecentlyUsed_SameUnitSize_EmptyExp( testParams->iterCount, testParams->maxItems, testParams->maxItemSize, testParams->threadID );
					break;
				default:
					assert( false );
			}
			break;
		}
		case USE_FREQUENTANDINFREQUENT_RANDOMUNITSIZE:
		{
			switch ( testParams->usePerThreadAllocator )
			{
				case USE_PER_THREAD_ALLOCATOR:
					printf( "    running thread %zd with randomTest_FrequentAndInfrequent_RandomUnitSize_UsingPerThreadAllocatorExp() ...\n", testParams->threadID );
					randomTest_FrequentAndInfrequent_RandomUnitSize_UsingPerThreadAllocatorExp( testParams->iterCount, testParams->maxItems, testParams->maxItemSize, testParams->maxItems2, testParams->maxItemSize2, testParams->threadID );
					break;
				case USE_NEW_DELETE:
					printf( "    running thread %zd with randomTest_FrequentAndInfrequent_RandomUnitSize_UsingNewAndDeleteExp() ...\n", testParams->threadID );
					randomTest_FrequentAndInfrequent_RandomUnitSize_UsingNewAndDeleteExp( testParams->iterCount, testParams->maxItems, testParams->maxItemSize, testParams->maxItems2, testParams->maxItemSize2, testParams->threadID );
					break;
				case USE_EMPTY_TEST:
					printf( "    running thread %zd with randomTest_FrequentAndInfrequent_RandomUnitSize_EmptyExp() ...\n", testParams->threadID );
					randomTest_FrequentAndInfrequent_RandomUnitSize_EmptyExp( testParams->iterCount, testParams->maxItems, testParams->maxItemSize, testParams->maxItems2, testParams->maxItemSize2, testParams->threadID );
					break;
				default:
					assert( false );
			}
			break;
		}
		case USE_FREQUENTANDINFREQUENT_SKEWEDBINSELECTION_RANDOMUNITSIZE:
		{
			switch ( testParams->usePerThreadAllocator )
			{
				case USE_PER_THREAD_ALLOCATOR:
					printf( "    running thread %zd with randomTest_FrequentAndInfrequent_SkewedBinSelection_RandomUnitSize_UsingPerThreadAllocatorExp() ...\n", testParams->threadID );
					randomTest_FrequentAndInfrequent_SkewedBinSelection_RandomUnitSize_UsingPerThreadAllocatorExp( testParams->iterCount, testParams->maxItems, testParams->maxItemSize, testParams->maxItems2, testParams->maxItemSize2, testParams->threadID );
					break;
				case USE_NEW_DELETE:
					printf( "    running thread %zd with randomTest_FrequentAndInfrequent_SkewedBinSelection_RandomUnitSize_UsingNewAndDeleteExp() ...\n", testParams->threadID );
					randomTest_FrequentAndInfrequent_SkewedBinSelection_RandomUnitSize_UsingNewAndDeleteExp( testParams->iterCount, testParams->maxItems, testParams->maxItemSize, testParams->maxItems2, testParams->maxItemSize2, testParams->threadID );
					break;
				case USE_EMPTY_TEST:
					printf( "    running thread %zd with randomTest_FrequentAndInfrequent_SkewedBinSelection_RandomUnitSize_EmptyExp() ...\n", testParams->threadID );
					randomTest_FrequentAndInfrequent_SkewedBinSelection_RandomUnitSize_EmptyExp( testParams->iterCount, testParams->maxItems, testParams->maxItemSize, testParams->maxItems2, testParams->maxItemSize2, testParams->threadID );
					break;
				default:
					assert( false );
			}
			break;
		}
		case USE_FREQUENTANDINFREQUENTWITHACCESS_RANDOMUNITSIZE:
		{
			switch ( testParams->usePerThreadAllocator )
			{
				case USE_PER_THREAD_ALLOCATOR:
					printf( "    running thread %zd with randomTest_FrequentAndInfrequentWithAccess_RandomUnitSize_UsingPerThreadAllocatorExp() ...\n", testParams->threadID );
					randomTest_FrequentAndInfrequentWithAccess_RandomUnitSize_UsingPerThreadAllocatorExp( testParams->iterCount, testParams->maxItems, testParams->maxItemSize, testParams->maxItems2, testParams->maxItemSize2, testParams->memReadCnt, testParams->threadID );
					break;
				case USE_NEW_DELETE:
					printf( "    running thread %zd with randomTest_FrequentAndInfrequentWithAccess_RandomUnitSize_UsingNewAndDeleteExp() ...\n", testParams->threadID );
					randomTest_FrequentAndInfrequentWithAccess_RandomUnitSize_UsingNewAndDeleteExp( testParams->iterCount, testParams->maxItems, testParams->maxItemSize, testParams->maxItems2, testParams->maxItemSize2, testParams->memReadCnt, testParams->threadID );
					break;
				case USE_EMPTY_TEST:
					printf( "    running thread %zd with randomTest_FrequentAndInfrequentWithAccess_RandomUnitSize_EmptyExp() ...\n", testParams->threadID );
					randomTest_FrequentAndInfrequentWithAccess_RandomUnitSize_EmptyExp( testParams->iterCount, testParams->maxItems, testParams->maxItemSize, testParams->maxItems2, testParams->maxItemSize2, testParams->memReadCnt, testParams->threadID );
					break;
				default:
					assert( false );
			}
			break;
		}
		case USE_FREQUENTANDINFREQUENTWITHACCESS_SKEWEDBINSELECTION_RANDOMUNITSIZE:
		{
			switch ( testParams->usePerThreadAllocator )
			{
				case USE_PER_THREAD_ALLOCATOR:
					printf( "    running thread %zd with randomTest_FrequentAndInfrequentWithAccess_SkewedBinSelection_RandomUnitSize_UsingPerThreadAllocatorExp() ...\n", testParams->threadID );
					randomTest_FrequentAndInfrequentWithAccess_SkewedBinSelection_RandomUnitSize_UsingPerThreadAllocatorExp( testParams->iterCount, testParams->maxItems, testParams->maxItemSize, testParams->maxItems2, testParams->maxItemSize2, testParams->memReadCnt, testParams->threadID );
					break;
				case USE_NEW_DELETE:
					printf( "    running thread %zd with randomTest_FrequentAndInfrequentWithAccess_SkewedBinSelection_RandomUnitSize_UsingNewAndDeleteExp() ...\n", testParams->threadID );
					randomTest_FrequentAndInfrequentWithAccess_SkewedBinSelection_RandomUnitSize_UsingNewAndDeleteExp( testParams->iterCount, testParams->maxItems, testParams->maxItemSize, testParams->maxItems2, testParams->maxItemSize2, testParams->memReadCnt, testParams->threadID );
					break;
				case USE_EMPTY_TEST:
					printf( "    running thread %zd with randomTest_FrequentAndInfrequentWithAccess_SkewedBinSelection_RandomUnitSize_EmptyExp() ...\n", testParams->threadID );
					randomTest_FrequentAndInfrequentWithAccess_SkewedBinSelection_RandomUnitSize_EmptyExp( testParams->iterCount, testParams->maxItems, testParams->maxItemSize, testParams->maxItems2, testParams->maxItemSize2, testParams->memReadCnt, testParams->threadID );
					break;
				default:
					assert( false );
			}
			break;
		}
		default:
			assert( false );
	}

	return nullptr;
}

void doTest( size_t testThreadCount, TestStartupParams* startupParams )
{
	TestStartupParams testParams[max_threads];
	std::thread threads[ max_threads ];

	for ( size_t i=0; i<testThreadCount; ++i )
	{
		memcpy( testParams + i, startupParams, sizeof(TestStartupParams) );
		testParams[i].threadID = i + 1;
	}
	// run thread
	for ( size_t i=0; i<testThreadCount; ++i )
	{
		printf( "about to run thread %zd...\n", i );
		std::thread t1( runRandomTest, (void*)(testParams + i) );
		threads[i] = std::move( t1 );
//		t1.detach();
		printf( "    ...done\n" );
	}
	// join threads
	for ( size_t i=0; i<testThreadCount; ++i )
	{
		printf( "joining thread %zd...\n", i );
		threads[i].join();
		printf( "    ...done\n" );
	}
}

struct TestRes
{
	size_t threadCount;
	size_t durEmpty;
	size_t durNewDel;
	size_t durPerThreadAlloc;
};

void runComparisonTest( size_t threadCount, TestStartupParams& params, TestRes& testRes )
{
	size_t start, end;
	testRes.threadCount = threadCount;

	params.usePerThreadAllocator = USE_EMPTY_TEST;

	start = GetMillisecondCount();
	doTest( threadCount, &params );
	end = GetMillisecondCount();
	testRes.durEmpty = end - start;
	printf( "%zd threads made %zd alloc/dealloc operations in %zd ms (%zd ms per 1 million)\n", threadCount, params.iterCount * threadCount, end - start, (end - start) * 1000000 / (params.iterCount * threadCount) );

	params.usePerThreadAllocator = USE_NEW_DELETE;

	start = GetMillisecondCount();
	doTest( threadCount, &params );
	end = GetMillisecondCount();
	testRes.durNewDel = end - start;
	printf( "%zd threads made %zd alloc/dealloc operations in %zd ms (%zd ms per 1 million)\n", threadCount, params.iterCount * threadCount, end - start, (end - start) * 1000000 / (params.iterCount * threadCount) );

	params.usePerThreadAllocator = USE_PER_THREAD_ALLOCATOR;

	start = GetMillisecondCount();
	doTest( threadCount, &params );
	end = GetMillisecondCount();
	testRes.durPerThreadAlloc = end - start;
	printf( "%zd threads made %zd alloc/dealloc operations in %zd ms (%zd ms per 1 million)\n", threadCount, params.iterCount * threadCount, end - start, (end - start) * 1000000 / (params.iterCount * threadCount) );

	printf( "Performance summary: %zd threads, (%zd - %zd) / (%zd - %zd) = %f\n\n\n", threadCount, testRes.durNewDel, testRes.durEmpty, testRes.durPerThreadAlloc, testRes.durEmpty, (testRes.durNewDel - testRes.durEmpty) * 1. / (testRes.durPerThreadAlloc - testRes.durEmpty) );
}

extern size_t commitCtr;
extern size_t decommitCtr;
extern size_t commitSz;
extern size_t decommitSz;
extern size_t maxAllocated;
int main()
{
	TestRes testRes[max_threads];

	if( 0 )
	{
		TestStartupParams params;
		params.iterCount = 100000000;
		params.maxItemSize = 8;
		params.maxItems = 10;
		params.maxItemSize2 = 16;
		params.maxItems2 = 16;
		params.memReadCnt = 0;
		params.calcMod = USE_RANDOMPOS_RANDOMSIZE;

		size_t threadCountMax = 3;

		for ( size_t threadCount=1; threadCount<=threadCountMax; ++threadCount )
			runComparisonTest( threadCount, params, testRes[threadCount] );

		printf( "Test summary for USE_RANDOMPOS_RANDOMSIZE:\n" );
		for ( size_t threadCount=1; threadCount<=threadCountMax; ++threadCount )
			printf( "%zd,%zd,%zd,%zd,%f\n", testRes[threadCount].threadCount, testRes[threadCount].durEmpty, testRes[threadCount].durNewDel, testRes[threadCount].durPerThreadAlloc, (testRes[threadCount].durNewDel - testRes[threadCount].durEmpty) * 1. / (testRes[threadCount].durPerThreadAlloc - testRes[threadCount].durEmpty) );
	}

	if( 0 )
	{
		TestStartupParams params;
		params.iterCount = 500000000;
		params.maxItemSize = 16;
		params.maxItems = 8;
		params.maxItemSize2 = 16;
		params.maxItems2 = 16;
		params.memReadCnt = 0;
		params.calcMod = USE_FREQUENTANDINFREQUENT_SKEWEDBINSELECTION_RANDOMUNITSIZE;

		size_t threadCountMax = 23;

		for ( size_t threadCount=1; threadCount<=threadCountMax; ++threadCount )
			runComparisonTest( threadCount, params, testRes[threadCount] );

		printf( "Test summary for USE_FREQUENTANDINFREQUENT_SKEWEDBINSELECTION_RANDOMUNITSIZE:\n" );
		for ( size_t threadCount=1; threadCount<=threadCountMax; ++threadCount )
			printf( "%zd,%zd,%zd,%zd,%f\n", testRes[threadCount].threadCount, testRes[threadCount].durEmpty, testRes[threadCount].durNewDel, testRes[threadCount].durPerThreadAlloc, (testRes[threadCount].durNewDel - testRes[threadCount].durEmpty) * 1. / (testRes[threadCount].durPerThreadAlloc - testRes[threadCount].durEmpty) );
	}

	if( 0 )
	{
		TestStartupParams params;
		params.iterCount = 10000000;
		params.maxItemSize = 16;
		params.maxItems = 8;
		params.maxItemSize2 = 16;
		params.maxItems2 = 16;
		params.memReadCnt = 16;
		params.calcMod = USE_FREQUENTANDINFREQUENTWITHACCESS_RANDOMUNITSIZE;

		size_t threadCountMax = 23;

		for ( size_t threadCount=1; threadCount<=threadCountMax; ++threadCount )
			runComparisonTest( threadCount, params, testRes[threadCount] );

		printf( "Test summary for USE_FREQUENTANDINFREQUENTWITHACCESS_RANDOMUNITSIZE, memReadCnt = %zd:\n", params.memReadCnt );
		for ( size_t threadCount=1; threadCount<=threadCountMax; ++threadCount )
			printf( "%zd,%zd,%zd,%zd,%f\n", testRes[threadCount].threadCount, testRes[threadCount].durEmpty, testRes[threadCount].durNewDel, testRes[threadCount].durPerThreadAlloc, (testRes[threadCount].durNewDel - testRes[threadCount].durEmpty) * 1. / (testRes[threadCount].durPerThreadAlloc - testRes[threadCount].durEmpty) );
	}

	if( 0 )
	{
		TestStartupParams params;
		params.iterCount = 10000000;
		params.maxItemSize = 16;
		params.maxItems = 8;
		params.maxItemSize2 = 16;
		params.maxItems2 = 16;
		params.memReadCnt = 16;
		params.calcMod = USE_FREQUENTANDINFREQUENTWITHACCESS_SKEWEDBINSELECTION_RANDOMUNITSIZE;

		size_t threadCountMax = 23;

		for ( size_t threadCount=1; threadCount<=threadCountMax; ++threadCount )
			runComparisonTest( threadCount, params, testRes[threadCount] );

		printf( "Test summary for USE_FREQUENTANDINFREQUENTWITHACCESS_SKEWEDBINSELECTION_RANDOMUNITSIZE, memReadCnt = %zd:\n", params.memReadCnt );
		for ( size_t threadCount=1; threadCount<=threadCountMax; ++threadCount )
			printf( "%zd,%zd,%zd,%zd,%f\n", testRes[threadCount].threadCount, testRes[threadCount].durEmpty, testRes[threadCount].durNewDel, testRes[threadCount].durPerThreadAlloc, (testRes[threadCount].durNewDel - testRes[threadCount].durEmpty) * 1. / (testRes[threadCount].durPerThreadAlloc - testRes[threadCount].durEmpty) );
	}

	if( 1 )
	{
		TestStartupParams params;
		params.iterCount = 1000000;
		params.maxItemSize = 18;
		params.maxItems = 10;
		params.maxItemSize2 = 12;
		params.maxItems2 = 16;
		params.memReadCnt = 16;
		params.calcMod = USE_RANDOMPOS_FIXEDSIZE;

		size_t threadCountMax = 3;

		for ( size_t threadCount=1; threadCount<=threadCountMax; ++threadCount )
			runComparisonTest( threadCount, params, testRes[threadCount] );

		printf( "Test summary for USE_RANDOMPOS_FIXEDSIZE, memReadCnt = %zd:\n", params.memReadCnt );
		for ( size_t threadCount=1; threadCount<=threadCountMax; ++threadCount )
			printf( "%zd,%zd,%zd,%zd,%f\n", testRes[threadCount].threadCount, testRes[threadCount].durEmpty, testRes[threadCount].durNewDel, testRes[threadCount].durPerThreadAlloc, (testRes[threadCount].durNewDel - testRes[threadCount].durEmpty) * 1. / (testRes[threadCount].durPerThreadAlloc - testRes[threadCount].durEmpty) );
	}

	if( 0 )
	{
		TestStartupParams params;
		params.iterCount = 1000000;
		params.maxItemSize = 18;
		params.maxItems = 10;
		params.maxItemSize2 = 12;
		params.maxItems2 = 16;
		params.memReadCnt = 16;
		params.calcMod = USE_RANDOMPOS_FULLMEMACCESS_FIXEDSIZE;

		size_t threadCountMax = 1;

		for ( size_t threadCount=1; threadCount<=threadCountMax; ++threadCount )
			runComparisonTest( threadCount, params, testRes[threadCount] );

		printf( "Test summary for USE_RANDOMPOS_FULLMEMACCESS_FIXEDSIZE, memReadCnt = %zd:\n", params.memReadCnt );
		for ( size_t threadCount=1; threadCount<=threadCountMax; ++threadCount )
			printf( "%zd,%zd,%zd,%zd,%f\n", testRes[threadCount].threadCount, testRes[threadCount].durEmpty, testRes[threadCount].durNewDel, testRes[threadCount].durPerThreadAlloc, (testRes[threadCount].durNewDel - testRes[threadCount].durEmpty) * 1. / (testRes[threadCount].durPerThreadAlloc - testRes[threadCount].durEmpty) );
	}

	printf( "commitCtr = %zd (%zd bytes), decommitCtr = %zd (%zd bytes), difference: %d (%d), maxAllocated = %zd\n", commitCtr, commitSz, decommitCtr, decommitSz, int(commitCtr - decommitCtr), int(commitSz - decommitSz), maxAllocated );

	return 0;
}
