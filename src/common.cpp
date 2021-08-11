/**
 * @file common.cpp
 * @author Jaroslav Janos (janosjar@fel.cvut.cz)
 * @brief 
 * @version 2.0
 * @date 04. 08. 2021
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#include "common.h"

// NON-SYMMETRIC variant of distance matrix for Dubins problem
template<>
class DistanceMatrix<DistanceHolder<Node<Point2DDubins>>> {
  public:
    DistanceMatrix(const int size) {
      holder.resize(size * size);
      this->size = size;
    }

    DistanceHolder<Node<Point2DDubins>>& operator()(int i, int j) {
      return holder[i * size + j];
    }

    const bool Exists(int i, int j) {
      return this->operator()(i, j).Exists();
    }

  private:
    std::deque<DistanceHolder<Node<Point2DDubins>>> holder;
    int size;
};

template<>
class DistanceMatrix<std::deque<DistanceHolder<Node<Point2DDubins>>>> {
  public:
    DistanceMatrix(const int size) {
      holder.resize(size * size);
      this->size = size;
    }

    std::deque<DistanceHolder<Node<Point2DDubins>>>& operator()(int i, int j) {
      return holder[i * size + j];
    }

    const bool Exists(int i, int j) {
      return !this->operator()(i, j).empty();
    }

  private:
    std::deque<std::deque<DistanceHolder<Node<Point2DDubins>>>> holder;
    int size;
};

FileStruct PrefixFileName(const FileStruct &path, const std::string &insert) {
	FileStruct retVal{path};

	auto pos{retVal.fileName.find_last_of("//")};
  if (pos != std::string::npos) {
    retVal.fileName.insert(pos + 1, insert);
  } else {
    retVal.fileName.insert(0, insert);
  }

	return retVal;
}

std::string Ltrim(const std::string &s) {
  size_t start{s.find_first_not_of(WHITESPACE)};
  return (start == std::string::npos) ? "" : s.substr(start);
}

std::string Rtrim(const std::string &s) {
  size_t end{s.find_last_not_of(WHITESPACE)};
  return (end == std::string::npos) ? "" : s.substr(0, end + 1);
}

std::string Trim(const std::string &s) {
  return Ltrim(Rtrim(s));
}

int ParseString(std::string &inp, std::string &outp1, std::string &outp2, std::string &delimiter) {
  size_t pos = inp.find(delimiter);
  int delimSize{static_cast<int>(delimiter.size())};
  int miss{1};
  if (pos != std::string::npos) {
    while (inp[pos + miss] == delimiter[miss]) {
      ++miss;
    }
    outp1 = inp.substr(0, pos);
    outp2 = inp.substr(pos + miss, inp.length());
    return pos;
  } else {
    outp1 = inp.substr(0, inp.length());
    outp2 = "";
    return -1;
  }
}
