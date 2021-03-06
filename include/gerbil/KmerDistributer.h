/*********************************************************************************
Copyright (c) 2016 Marius Erbert, Steffen Rechner

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*********************************************************************************/

#ifndef KMERDISTRIBUTER_H_
#define KMERDISTRIBUTER_H_

#include <cstdint>
#include <unistd.h>
#include <atomic>
#include "KMer.h"

namespace gerbil {

/**
 * Attention! Not thread-safe!
 * This class oversees the distribution of kmers to various hasher threads.
 * It calculates the number of kmers for each thread for certain files.
 */
class KmerDistributer {

private:

	// constants
	const uint32_t BUCKETSIZE = 512;		// should be a power of two
	const uint32_t numThreadsCPU;			// number of CPU hasher threads
	const uint32_t numThreadsGPU;			// number of GPU hasher threads

	// global statistic information
	uint64_t *capacity;						// the maximal number of kmers for each hasher thread
	float* throughput;						// the throughput of each hasher thread

	// working memory
	float *prefixSum;						// the prefix sums of the ratio field
	float* tmp;								// working memory
	uint32_t lastFileId = -1;

	// file-specific variables
	std::vector<uint32_t*> buckets;			// distribution of kmers over the various hasher threads
	std::vector<int> lock;					// whether or not the distribution for file i has beeen computed yet
	std::vector<float*> ratio;				// a ratio between 0 and 1 for each hasher thread and file

public:
	KmerDistributer(const uint32_t numCPUHasherThreads,
			const uint32_t numGPUHasherThreads, const uint32_t maxNumFiles);
	virtual ~KmerDistributer();

	/**
	 * Construct a kmer distribution for a new temporary file.
	 */
	void updateFileInformation(const uint32_t curFileId,
			const uint64_t kmersInFile);

	/**
	 * Updates the Throughput of a hasher thread.
	 */
	void updateThroughput(const bool gpu, const uint32_t tId, const float throughput);

	/**
	 * Updates the kmer capacity of a hasher thread.
	 */
	void updateCapacity(const bool gpu, const uint32_t tId, const uint64_t cap);

	/**
	 * Returns the ratio of each hasher thread for a certain file
	 */
	float getSplitRatio(const bool gpu, const uint32_t tId,
			const uint_tfn curTempFileId) const;

	/**
	 * Determines the bucket id of a kmer.
	 * @param kmer The Kmer that should be hashed
	 * @return A number betweeen zero and the total number
	 * of hasher threads.
	 */
	template<uint32_t K>
	uint32_t inline distributeKMer(const KMer<K>& kmer,
			const uint32_t curFileId) const {

	//	return numThreadsCPU;

		// wait until updates are complete
		while (lock[curFileId])
			usleep(10);

		// Determine hash value of kmer by table lookup
		const uint32_t hash = kmer.getPartHash() % BUCKETSIZE;
		//const uint32_t hash = kmer.getPartHash() & (BUCKETSIZE-1);
		return buckets[curFileId][hash];
	}
};

} /* namespace gerbil */

#endif /* KMERDISTRIBUTER_H_ */
