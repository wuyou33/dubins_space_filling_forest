#pragma once
/**
 * @file primitives.h
 * @author Jaroslav Janos (janosjar@fel.cvut.cz)
 * @brief 
 * @version 2.0
 * @date 02. 08. 2021
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#ifndef __COMMON_H__
#define __COMMON_H__

#include <math.h>
#include <regex>
#include <vector>
#include <deque>
#include <chrono>
#include <limits.h>
#include <flann/flann.hpp>
#include "point-types.h"
#include "vector-types.h"
#include "constants.h"

#define PROBLEM_DIMENSION   NumDimensions[this->problem.Dimension]

#define MIN(X, Y) ((X < Y) ? (X) : (Y))
#define MAX(X, Y) ((X > Y) ? (X) : (Y))
#define SQR(X)    ((X) * (X))

#define ERROR(mess)  std::cerr << "[\033[1;31m ERR\033[0m ]  " << mess << "\n"
#define INFO(mess)   std::cout << "[\033[1;34m INF\033[0m ]  " << mess << "\n"
#define WARN(mess)   std::cerr << "[\033[1;33m WAR\033[0m ]  " << mess << "\n"

struct FileStruct;
template<class R> struct DistanceHolder;
template<class R> class Node;

int ParseString(std::string &inp, std::string &outp1, std::string &outp2, std::string &delimiter);
FileStruct PrefixFileName(const FileStruct &path, const std::string &insert);
std::string Ltrim(const std::string &s);
std::string Rtrim(const std::string &s);
std::string Trim(const std::string &s);
std::string ToLower(std::string s);

// FLANN FUNCTOR
template<class T>
struct D6Distance {
  typedef bool is_vector_space_distance;

  typedef T ElementType;
  typedef typename flann::Accumulator<T>::Type ResultType;

  template <typename Iterator1, typename Iterator2>
  ResultType operator()(Iterator1 a, Iterator2 b, size_t size, ResultType worst_dist = -1) const {
    ResultType result = ResultType();
    ResultType diff;

    for (int i{0}; i < 3; ++i) {
      diff = (ResultType)(*a++ - *b++);
      result = diff * diff;
    }

    Quaternion q1, q2;
    for (int i{3}; i < 7; ++i) {
      q1.Set(i-3, *a++);
      q2.Set(i-3, *b++);
    }

    diff = q1.Distance(q2);
    result += diff * diff;
    return result;
  }
};

enum Dimensions {
  D2 = 0,
  D2Dubins = 1,
  D3 = 2,
  D3Dubins = 3
};

const static inline size_t NumDimensions[] = { 2, 3, 3, 6 };

enum FileType {
  Map,
  Obj
};

enum SaveOptions {
  None = 0,
  SaveGoals = 1,
  SaveTree = 2,
  SaveRoadmap = 4,
  SaveParams = 8,
  SaveTSPFile = 16,
  SaveFrontiers = 32,
  SaveTSPPaths = 64,
  Invalid = 128
};

enum SolverType{
  SFF,
  RRT,
  Lazy
};

enum TSPType {
  Concorde,
  LKH
};

struct FileStruct {
  std::string fileName;
  FileType type;
};

struct Range {
  double mins[3];
  double maxs[3];

  Range(double minX, double maxX, double minY, double maxY, double minZ, double maxZ) 
    : mins{minX, minY, minZ}, maxs{maxX, maxY, maxZ} {
    }

  void Parse(std::string &range, double scale, int order) {
    std::regex r("\\[(\\-?[\\d]+[\\.]?[\\d]*);\\s*(\\-?[\\d]+[\\.]?[\\d]*)\\]");
    std::smatch m;
    std::regex_search(range, m, r);
    if (m.size() != 3) {
      throw std::invalid_argument("Unknown format of range");
    }

    mins[order] = std::stod(m[1]) * scale;
    maxs[order] = std::stod(m[2]) * scale;
  }
};

template<class R>
struct DistanceHolder {
  Node<R> *Node1;
  Node<R> *Node2;
  double Distance;
  bool IsValid{false};
  std::deque<R> Plan;

  DistanceHolder() : Node1{NULL}, Node2{NULL}, Distance{std::numeric_limits<double>::max()} {
  }

  DistanceHolder(Node<R> *first, Node<R> *second, bool computeDistance=false) : Node1{first}, Node2{second} {
    if (*first < *second) {
      Node1 = first;
      Node2 = second;
    } else {
      Node1 = second;
      Node2 = first;
    }
    if (computeDistance) {
      this->UpdateDistance();
    }
  }

  DistanceHolder(Node<R> *first, Node<R> *second, double dist) : Distance{dist} {
    if (*first < *second) {
      Node1 = first;
      Node2 = second;
    } else {
      Node1 = second;
      Node2 = first;
    }
  }

  DistanceHolder(Node<R> *first, Node<R> *second, double dist, std::deque<R> &plan) : Distance{dist}, Plan{plan} {
    if (*first < *second) {
      Node1 = first;
      Node2 = second;
    } else {
      Node1 = second;
      Node2 = first;
      std::reverse(this->Plan.begin(), this->Plan.end());
    }
  }

  friend bool operator<(const DistanceHolder<R> &l, const DistanceHolder<R> &r) {
    return l.Distance < r.Distance;
  }  

  friend bool operator==(const DistanceHolder<R> &l, const DistanceHolder<R> &r) {
    return (l.Node1 == r.Node1 && l.Node2 == r.Node2) || (l.Node1 == r.Node2 && l.Node2 == r.Node1);
  }

  const bool Exists() const {
    return Node1 != nullptr;
  }

  void UpdateDistance() {
    Distance = Node1->DistanceToRoot() + Node2->DistanceToRoot() + Node1->Position.Distance(Node2->Position);
  }
};

template<>
struct DistanceHolder<Point2DDubins> {
  Node<Point2DDubins> *Node1;
  Node<Point2DDubins> *Node2;
  double Distance;
  bool IsValid{false};
  std::deque<Point2DDubins> Plan;

  DistanceHolder() : Node1{NULL}, Node2{NULL}, Distance{std::numeric_limits<double>::max()} {
  }

  DistanceHolder(Node<Point2DDubins> *first, Node<Point2DDubins> *second, bool computeDistance=false) : Node1{first}, Node2{second} {
    Node1 = first;
    Node2 = second;
    
    if (computeDistance) {
      this->UpdateDistance();
    }
  }

  DistanceHolder(Node<Point2DDubins> *first, Node<Point2DDubins> *second, double dist) : Distance{dist} {
    Node1 = first;
    Node2 = second;
  }

  DistanceHolder(Node<Point2DDubins> *first, Node<Point2DDubins> *second, double dist, std::deque<Point2DDubins> &plan) : Distance{dist}, Plan{plan} {
    Node1 = first;
    Node2 = second;
  }

  friend bool operator<(const DistanceHolder<Point2DDubins> &l, const DistanceHolder<Point2DDubins> &r) {
    return l.Distance < r.Distance;
  }  

  friend bool operator==(const DistanceHolder<Point2DDubins> &l, const DistanceHolder<Point2DDubins> &r) {
    return (l.Node1 == r.Node1 && l.Node2 == r.Node2) || (l.Node1 == r.Node2 && l.Node2 == r.Node1);
  }

  const bool Exists() const {
    return Node1 != nullptr;
  }

  void UpdateDistance(int angleId1=-1, int angleId2=-1);
};

template<class T>
class DistanceMatrix {
  public:
    DistanceMatrix() {
    }

    DistanceMatrix(const int size) {
      holder.resize(size * (size+1) / 2);
      this->size = size;
    }

    T& operator()(int i, int j) {
      int index;
      if (i <= j) {
        index = i * size - (i - 1) * i / 2 + j - i;
      } else {
        index = j * size - (j - 1) * j / 2 + i - j;
      }
      return holder[index];
    }

    const bool Exists(int i, int j) {
      return this->operator()(i, j).Exists();
    }

    const int GetSize() {
      return size;
    }

  private:
    std::deque<T> holder;
    int size;
};

// Distance matrix for Dubins problem = 4D matrix considering inlet/outlet angles
template<>
class DistanceMatrix<DistanceHolder<Point2DDubins>> {
  public:
    DistanceMatrix() {
    }

    DistanceMatrix(const int size, const int angleResolution) {
      refMatrix.resize(size);
      for (int i{0}; i < size; ++i) {
        refMatrix[i].resize(size);
        for (int j{0}; j < size; ++j) {
          refMatrix[i][j].resize(angleResolution);
          for (int k{0}; k < angleResolution; ++k) {
            refMatrix[i][j][k].resize(angleResolution, -1);
          }
        }
      }

      this->size = size;
      this->angleResolution = angleResolution;
    }

    DistanceHolder<Point2DDubins>& operator()(int id1, int id2, int angleId1, int angleId2) {
      return holder[refMatrix[id1][id2][angleId1][angleId2]];
    }

    const bool Exists(int id1, int id2, int angleId1, int angleId2) {
      return refMatrix[id1][id2][angleId1][angleId2] != -1;
    }

    void AddLink(DistanceHolder<Point2DDubins> &link, int id1, int id2, int angleId1, int angleId2, bool secondIsInlet=false) {
      holder.push_back(std::move(link));
      int holderId{static_cast<int>(holder.size() - 1)};

      if (!secondIsInlet) {
        refMatrix[id1][id2][angleId1][OppositeAngleID(angleId2)] = holderId;
      } else {
        refMatrix[id1][id2][angleId1][angleId2] = holderId;
      }
      
    }

    std::deque<DistanceHolder<Point2DDubins>> &GetHolder() {
      return holder;
    }

    const int OppositeAngleID(const int angleID) {
      return (angleID + angleResolution / 2) % angleResolution;
    }

    const int GetSize() {
      return size;
    }

    const int GetResolution() {
      return angleResolution;
    }

  private:
    std::deque<DistanceHolder<Point2DDubins>> holder;
    std::deque<std::deque<std::deque<std::deque<int>>>> refMatrix;
    int size;
    int angleResolution;
};

template<>
class DistanceMatrix<std::deque<DistanceHolder<Point2DDubins>>> {
  public:
    DistanceMatrix() {
    }
    
    DistanceMatrix(const int size, const int angleResolution) {
      refMatrix.resize(size);
      for (int i{0}; i < size; ++i) {
        refMatrix[i].resize(size);
        for (int j{0}; j < size; ++j) {
          refMatrix[i][j].resize(angleResolution);
          for (int k{0}; k < angleResolution; ++k) {
            refMatrix[i][j][k].resize(angleResolution);
          }
        }
      }

      this->size = size;
      this->angleResolution = angleResolution;
    }

    std::deque<DistanceHolder<Point2DDubins>> &operator()(int id1, int id2, int angleId1, int angleId2) {
      return refMatrix[id1][id2][angleId1][angleId2];
    }

    const bool Exists(int id1, int id2, int angleId1, int angleId2) {
      return !this->operator()(id1, id2, angleId1, angleId2).empty();
    }

    void AddLink(DistanceHolder<Point2DDubins> &link, int id1, int id2, int angleId1, int angleId2, bool secondIsInlet=false) {
      if (!secondIsInlet) {
        // both trees' angles are considered as outlet angles -- inlet angles are the opposite ones
        refMatrix[id1][id2][angleId1][OppositeAngleID(angleId2)].push_back(link);
      } else {
        refMatrix[id1][id2][angleId1][angleId2].push_back(link);
      }
      
    }

    const int OppositeAngleID(const int angleID) {
      return (angleID + angleResolution / 2) % angleResolution;
    }

    const int GetSize() {
      return size;
    }

  private:
    std::deque<std::deque<std::deque<std::deque<std::deque<DistanceHolder<Point2DDubins>>>>>> refMatrix;
    int angleResolution;
    int size;
};

template<class R>
class UnionFind {
  public:
    UnionFind() {
    }

    UnionFind(std::deque<R> &elements) {
      for (auto r : elements) {
        holder[r] = r;
        counter[r] = 0;
      }
    }

    void Union(R child, R parent) {
      counter[Find(parent)] += counter[Find(child)] + 1; 
      holder[Find(child)] = parent;
    }

    R Find(R element) {
      R parent = holder[element];
      if (parent != element) {
        holder[element] = Find(parent);
      }

      return holder[element];
    }

    std::deque<R> GetAllWithParent(R parent) {
      std::deque<R> retVal;
      for (auto iter{holder.begin()}; iter != holder.end(); iter++) {
        if (Find(iter->second) == parent) {
          retVal.push_back(iter->first);
        }
      }

      return retVal;
    }

    int GetCountOf(R element) {
      return counter[element];
    }

  private:
    std::map<R, R> holder;
    std::map<R, int> counter;
};

class StopWatch {
  public:
    void Start();
    void Stop();
    std::chrono::duration<double> GetElapsed();
  private:
    std::chrono::high_resolution_clock::time_point startTime;
    std::chrono::high_resolution_clock::time_point stopTime;
};

template <typename T> 
int sgn(T val) {
    return (T(0) < val) - (val < T(0));
}

template <class T>
T NormalizeAngle(T angle) {
  if (angle < -M_PI) {
    return angle + 2 * M_PI;
  } else if (angle >= M_PI) {
    return angle - 2 * M_PI;
  } else {
    return angle;
  }
}

template <class T>
T AngleDifference(T a1, T a2) {
  double angleDirection{a1 - a2};
  if (a1 >= a2) {
    if (angleDirection < M_PI) {
      angleDirection *= -1;
    } else {
      angleDirection = 2 * M_PI - angleDirection;
    }
  } else {
    if (angleDirection < -M_PI) {
      angleDirection = -2 * M_PI - angleDirection;
    } else {
      angleDirection *= -1; 
    }
  }
  return angleDirection;
}

/**
 * @brief Addition of a flag to save options
 * 
 * @param a Current save options
 * @param b Flag to be added
 * @return SaveOptions New save options 
 */
inline SaveOptions operator+(SaveOptions a, SaveOptions b) {
  return static_cast<SaveOptions>(static_cast<int>(a) | static_cast<int>(b));
}

inline SaveOptions operator-(SaveOptions a, SaveOptions b) {
  return static_cast<SaveOptions>(static_cast<int>(a) - static_cast<int>(b));
}

/**
 * @brief Returns if a flag in save options is active
 * 
 * @param a Flag to be determined
 * @param b Current save options
 * @return true Flag is active
 * @return false Otherwise
 */
inline bool operator<=(SaveOptions a, SaveOptions b) {
  return  (static_cast<int>(b) & static_cast<int>(a)) == static_cast<int>(a);
}

inline std::ostream& operator<<(std::ostream &out, const Dimensions &d) {
  if (d == D2) {
    return out << "2D";
  } else if (d == D2Dubins) {
    return out << "2DDubins";
  } else if (d == D3) {
    return out << "3D";
  } else if (d == D3Dubins) {
    return out << "3DDubins";
  } else {
    ERROR("Unimplemented dimension print!");
    return out;
  }
}

#include "graph-types.h"

#endif /* __COMMON_H__ */
