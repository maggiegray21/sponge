/*
 * Filename: stream_reassembler.cc
 * Author: Maggie Gray
 * Description: This file implements a StreamReassembler which reassembles Unassembled strings to
 * send to a ByteStream.
 */

#include "stream_reassembler.hh"
using namespace std;

/*
 * Function: checkOverlaps
 * Args: Unassembled u1, Unassembled u2
 * Description: This function takes in Unassembled structs and checks to see if their data overlaps.
 * If they overlap, it returns true. Otherwise, it returns false.
 */
bool StreamReassembler::checkOverlaps(Unassembled u1, Unassembled u2) {
    // u1 starts in the middle of u2
    if (u1.startIndex >= u2.startIndex && u1.startIndex <= u2.endIndex) {
        return true;
    }

    // u2 starts in the middle of u1
    if (u2.startIndex >= u1.startIndex && u2.startIndex <= u1.endIndex) {
        return true;
    }

    return false;
}

/*
 * Function: constructOverlapping
 * Args: Unassembled u1, Unassembled u2
 * Description: This function takes in two overlapping Unassembled structs and merges their data into
 * one Unassembled. It returns the merged Unassembed.
 */
Unassembled StreamReassembler::constructOverlapping(Unassembled u1, Unassembled u2) {
    Unassembled overlapping;
    string data = "";

    // set overlapping's start index to be the smaller start index of u1 and u2
    // copy in the beginning of the string
    int nextIndexNeeded = 0;
    if (u1.startIndex < u2.startIndex) {
        overlapping.startIndex = u1.startIndex;
        data += u1.str;
        nextIndexNeeded += u1.startIndex + u1.str.length();
    } else {
        overlapping.startIndex = u2.startIndex;
        data += u2.str;
        nextIndexNeeded += u2.startIndex + u2.str.length();
    }

    // set overlapping's end index to be the larger end index of u1 and u2
    // copy in the rest of the string if there is more to copy
    if (u1.endIndex > u2.endIndex) {
        overlapping.endIndex = u1.endIndex;
        if (overlapping.endIndex >= data.length()) {
            data += u1.str.substr(nextIndexNeeded - u1.startIndex);
        }
    } else {
        overlapping.endIndex = u2.endIndex;
        if (overlapping.endIndex >= data.length()) {
            data += u2.str.substr(nextIndexNeeded - u2.startIndex);
        }
    }

    overlapping.str = data;
    return overlapping;
}

/*
 * Function: addToList
 * Arg: Unassembled current
 * Description: This function takes in an Unassembled and adds it to our list of Unassembleds.
 * It combines the Unassembled with any elements of the list that it overlaps with and then adds it
 * to the list in a sorted order based on its first index.
 */
void StreamReassembler::addToList(Unassembled current) {
    // do not add it to the list if it is the empty string
    // if (current.str == "") return;

    // if the list is empty, add current to the front of the list and return
    if (unassembledList.empty()) {
        unassembledList.push_front(current);
        bytesInList += current.str.length();
        return;
    }

    // if the list is not empty, check if current data overlaps with existing data
    // insert into list sorted by index
    for (auto it = unassembledList.begin(); it != unassembledList.end(); it++) {
        Unassembled next = *it;
        if (checkOverlaps(current, next)) {
            Unassembled overlapping = constructOverlapping(current, next);

            // remove the unassembled data we overlap with
            auto nextIt = unassembledList.erase(it);
            bytesInList -= next.str.length();

            // While we are not at the end of our list, check to see if the next element
            // overlaps. If it does, combine the next element with overlapping and look
            // at the next element after it. If they do not overlap, break the while loop
            while (nextIt != unassembledList.end()) {
                Unassembled nextItUnass = *nextIt;
                if (checkOverlaps(overlapping, nextItUnass)) {
                    overlapping = constructOverlapping(overlapping, nextItUnass);
                    bytesInList -= nextItUnass.str.length();
                    nextIt = unassembledList.erase(nextIt);
                } else {
                    break;
                }
            }

            // insert new overlapping string into our list and return
            unassembledList.insert(nextIt, overlapping);
            bytesInList += overlapping.str.length();
            return;
        }

        // if our current unassembled chunk does not overlap with the next chunk and
        // its start index is smaller than the start index of the next chunk, then
        // insert it into the list and return
        if (current.startIndex < next.startIndex) {
            unassembledList.insert(it, current);
            bytesInList += current.str.length();
            return;
        }
    }
    // put it at the back otherwise
    unassembledList.push_back(current);
    bytesInList += current.str.length();
}

/*
 * Function: protected_write
 * Args: Unassembled current
 * Description: This function takes in an Unassembled and writes its data to the ByteStream _output.
 * It also checks to see if the file is EOF and increments the currentIndex variable.
 */
void StreamReassembler::protected_write(Unassembled current) {
    // write the string to the bytestream and increment our current index
    _output.write(current.str);
    currentIndex += current.str.length();

    // if we have hit the index where we hit EOF and EOF has been pushed, end the output
    if (currentIndex == eofIndex && eofPushed) {
        _output.end_input();
    }
}

/*
 * Function: write_substring
 * Args: Unassembled current
 * Description: This function takes in an Unassembled
 */
void StreamReassembler::write_substring(Unassembled current) {
    // check to see if our new unassembled chunk overlaps with the next chunk to
    // be written to the byte stream
    // if they overlap, write the combined chunk to the byte stream, otherwise write
    // the new data to the bytestream
    while (!unassembledList.empty() && checkOverlaps(current, unassembledList.front())) {
        Unassembled overlapping = constructOverlapping(current, unassembledList.front());
        bytesInList -= unassembledList.front().str.length();
        unassembledList.pop_front();
        current = overlapping;
    }

    protected_write(current);

    // check to see if the front of our list can be written
    // if it can be written, then write it and remove it from unassembledList
    while (!unassembledList.empty() && (currentIndex == unassembledList.front().startIndex)) {
        protected_write(unassembledList.front());
        bytesInList -= unassembledList.front().str.length();
        unassembledList.pop_front();
    }
}

/*
 * Function: createUnassembled
 * Args: const string &data, const size_t index
 * Description: This function takes in a string and an index and constructs an Unassembled
 * containing that string and start index. It returns an Unassembled.
 */
Unassembled StreamReassembler::createUnassembled(const string &data, const size_t index) {
    // create a new Unassembled chunk of data
    struct Unassembled current;
    current.startIndex = index;
    current.endIndex = index + data.length() - 1;

    // if we do not have capacity for the end of the string, discard it
    if (current.endIndex > maxEndIndex) {
        string str = data.substr(0, maxEndIndex + 1 - index);
        current.str.assign(str);
    } else {
        current.str.assign(data);
    }
    return current;
}

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) {
    // find the maximum end index a string can have, based on the reassembler's capacity
    maxEndIndex = _capacity + _output.bytes_read() - 1;

    // if data chunk has EOF set, set our global eofPushed variable to true
    // and record the index where we hit EOF
    if (eof) {
        eofPushed = true;
        eofIndex = index + data.length();
    }

    // if we have hit the index where we hit EOF, end the output and return
    if (currentIndex == eofIndex && eofPushed) {
        _output.end_input();
        return;
    }

    // if the string is empty, discard it
    if (data == "") {
        return;
    }

    // if we do not have capacity for the string, discard it
    if (index > maxEndIndex) {
        return;
    }

    // create a new Unassembled struct
    struct Unassembled current = createUnassembled(data, index);

    // if the index of the data is the next index, then write it to the byte stream

    if (index == currentIndex) {
        write_substring(current);
        return;
    }

    // if part (but not all) of the data chunk has already been written
    else if (current.startIndex < currentIndex && current.endIndex >= currentIndex) {
        // get the part of the string that has not yet been written
        string toWrite = current.str.substr(currentIndex - current.startIndex);
        current.str = toWrite;

        // set proper start index and write the Unassembled chunk of data
        current.startIndex = currentIndex;
        write_substring(current);
        return;
    }

    // if we have already written all the data in this chunk, return
    else if (current.startIndex < currentIndex && current.endIndex < currentIndex) {
        return;
    }

    // if we are not ready to write this data to the bytestream, and we have capacity
    // add it to our list of unassembled data
    else if (_capacity >= data.length()) {
        addToList(current);
    }
}

size_t StreamReassembler::unassembled_bytes() const { return bytesInList; }

// true if there are no unassembled bytes, false otherwise
bool StreamReassembler::empty() const { return (unassembled_bytes() == 0); }
