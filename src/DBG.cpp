#include "DBG.h"

using namespace std;

std::vector<std::string> getNeighbours(std::string kMer, unsigned merSize, int left, std::unordered_map<std::string, unsigned> merCounts, unsigned solidThresh) {
	std::vector<std::string> neighbours;
	std::string f, n;

	if (left == 0) {
		f = kMer.substr(1);
		for (char c : "ACGT") {
			n = f + c;
			if (merCounts[n] >= solidThresh) {
				neighbours.push_back(n);
			}
		}
	} else {
		f = kMer.substr(0, merSize - 1);
		for (char c : "ACGT") {
			n = c + f;
			if (merCounts[n] >= solidThresh) {
				neighbours.push_back(n);
			}
		}
	}

	// Sort in ascending order of number of occurrences
	std::sort(neighbours.begin(), neighbours.end(), 
		[&merCounts](std::string& n1, std::string& n2) {
			return  merCounts[n1] > merCounts[n2];
		}
	);
	
	return neighbours;
}

unsigned extendLeft(std::unordered_map<std::string, unsigned> merCounts, unsigned curK, unsigned extLen, string &LR, unsigned solidThresh) {
		vector<string> neighbours;
		vector<string>::iterator it;
		unsigned dist = 0;

		// Get the leftmost k-mer and search for a path in the graph
		neighbours = getNeighbours(LR.substr(0, curK), curK, 1, merCounts, solidThresh);
		it = neighbours.begin();

		// Keep traversing the graph while the long reads's border or a branching path aren't reached
		while (neighbours.size() == 1 && it != neighbours.end() && dist < extLen) {
			LR = (*it).substr(0, it->length() - (curK - 1)) + LR;
			dist = dist + it->length() - (curK - 1);
			// Get the leftmost k-mer and search for a path in the graph
			neighbours = getNeighbours(LR.substr(0, curK), curK, 1, merCounts, solidThresh);
			it = neighbours.begin();	
		}
		
		return dist;
	}

	unsigned extendRight(std::unordered_map<std::string, unsigned> merCounts, unsigned curK, unsigned extLen, string &LR, unsigned solidThresh) {
		vector<string> neighbours;
		vector<string>::iterator it;
		unsigned dist = 0;

		// Get the leftmost k-mer and search for a path in the graph
		neighbours = getNeighbours(LR.substr(LR.length() - curK), curK, 0, merCounts, solidThresh);
		it = neighbours.begin();
		
		// Keep traversing the graph while the long reads's border or a branching path aren't reached
		while (neighbours.size() == 1 && it != neighbours.end() && dist < extLen) {
			LR = LR + (*it).substr(curK - 1);
			dist = dist + it->length() - (curK - 1);
			// Get the leftmost k-mer and search for a path in the graph
			neighbours = getNeighbours(LR.substr(LR.length() - curK), curK, 0, merCounts, solidThresh);
			it = neighbours.begin();
		}
		
		return dist;
	}


int link(std::unordered_map<std::string, unsigned> merCounts, std::string srcSeed, std::string tgtSeed, unsigned curK, std::set<std::string> &visited, unsigned* curBranches, unsigned dist, std::string curExt, std::string &missingPart, unsigned merSize, unsigned LRLen, unsigned maxBranches, unsigned solidThresh, unsigned minOrder) {
	if (curK < minOrder || *curBranches > maxBranches || dist > LRLen) {
			missingPart = std::string();
			return 0;
	}
	
	std::string srcAnchor = curExt.substr(curExt.length() - curK);
	std::string tgtAnchor = tgtSeed.substr(0, curK);
	std::vector<std::string> neighbours;
	std::vector<std::string>::iterator it;
	bool found = srcAnchor == tgtAnchor;
	std::string curRead;
	std::string resPart1 = std::string(curExt);
	std::set<std::string>::iterator itf;
	
	// Search for a path in the graph starting from the source's anchor
	neighbours = getNeighbours(srcAnchor.substr(srcAnchor.length() - curK), curK, 0, merCounts, solidThresh);
	it = neighbours.begin();

	// While the destination or a braching path aren't reached, keep on traversing the graph
	while (!found && neighbours.size() == 1 && it != neighbours.end() && dist <= LRLen) {
		curRead = *it;
		// itf = visited.find(curRead);
		tgtAnchor = tgtSeed.substr(0, curRead.length());
		found = curRead == tgtAnchor;
		// if (!found && (itf == visited.end())) {
		if (!found) {
			// visited.insert(curRead);
			// resPart1 = resPart1 + curRead.substr(curK - 1);
			resPart1 = resPart1 + curRead[curK - 1];
			dist = dist + curRead.length() - (curK - 1);

			// Update the current k-mer, and search for a path in the graph
			srcAnchor = resPart1.substr(resPart1.length() - curK);
			neighbours = getNeighbours(srcAnchor.substr(srcAnchor.length() - curK), curK, 0, merCounts, solidThresh);
			it = neighbours.begin();
		} else if (found) {
			// resPart1 = resPart1 + curRead.substr(curK - 1);
			resPart1 = resPart1 + curRead[curK - 1];
		} else {
			it++;
		}
	}

	// If a branching path is reached, explore the different possible paths with backtracking
	while (!found && neighbours.size() > 1 && it != neighbours.end() && dist <= LRLen) {
		curRead = *it;
		// itf = visited.find(curRead);
		tgtAnchor = tgtSeed.substr(0, curRead.length());
		found = curRead == tgtAnchor;
		// if (!found && (itf == visited.end())) {
		if (!found) {
			// visited.insert(curRead);
			(*curBranches)++;
			// found = link(merCounts, srcSeed, tgtSeed, merSize, visited, curBranches, dist + curRead.length() - (curK - 1), resPart1 + curRead.substr(curK - 1), missingPart, merSize, LRLen, maxBranches, solidThresh, minOrder);
			found = link(merCounts, srcSeed, tgtSeed, merSize, visited, curBranches, dist + curRead.length() - (curK - 1), resPart1 + curRead[curK - 1], missingPart, merSize, LRLen, maxBranches, solidThresh, minOrder);
			if (!found) {
				++it;
			} else {
				return 1;
			}
		} else if (found) {
			resPart1 = resPart1 + curRead[curK - 1];
		} else {
			++it;
		}
	}
	
	// If the source couldn't be linked to the destination, try again with a graph of smaller order, otherwhise update the missing part and return
	if (!found) {
		return 0;
	} else {
		missingPart = resPart1 + tgtSeed.substr(curK);
		return 1;
	}
}