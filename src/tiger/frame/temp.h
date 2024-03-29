#ifndef TIGER_FRAME_TEMP_H_
#define TIGER_FRAME_TEMP_H_

#include "tiger/symbol/symbol.h"

#include <list>

namespace temp {

using Label = sym::Symbol;

class LabelFactory {
public:
  static Label *NewLabel();
  static Label *NamedLabel(std::string_view name);
  static std::string LabelString(Label *s);

private:
  int label_id_ = 0;
  static LabelFactory label_factory;
};

class Temp {
  friend class TempFactory;

public:
  // for GC
  bool pointer;
  [[nodiscard]] int Int() const;
  void setPointer() { pointer = true; }
  void discardPointer() { pointer = false; }
  bool isPointer() { return pointer; }

private:
  int num_;
  explicit Temp(int num, bool Pointer) : num_(num), pointer(Pointer) {}
};

class TempFactory {
public:
  static Temp *NewTemp(bool Pointer = false);

private:
  int temp_id_ = 100;
  static TempFactory temp_factory;
};

class Map {
public:
  void Enter(Temp *t, std::string *s);
  std::string *Look(Temp *t);
  void DumpMap(FILE *out);

  static Map *Empty();
  static Map *Name();
  static Map *LayerMap(Map *over, Map *under);

private:
  tab::Table<Temp, std::string> *tab_;
  Map *under_;

  Map() : tab_(new tab::Table<Temp, std::string>()), under_(nullptr) {}
  Map(tab::Table<Temp, std::string> *tab, Map *under)
      : tab_(tab), under_(under) {}
};

class TempList {
public:
  explicit TempList(Temp *t) : temp_list_({t}) {}
  TempList(std::initializer_list<Temp *> list) : temp_list_(list) {}
  TempList() = default;
  void Append(Temp *t) { temp_list_.push_back(t); }
  [[nodiscard]] Temp *NthTemp(int i) const;
  [[nodiscard]] const std::list<Temp *> &GetList() const { return temp_list_; }
  [[nodiscard]] bool Contain(Temp *temp) {
    return std::find(temp_list_.begin(), temp_list_.end(), temp) !=
           temp_list_.end();
  };
  void Replace(Temp *oldTemp, Temp *newTemp) {
    for (auto item = temp_list_.begin(); item != temp_list_.end(); item++) {
      if ((*item) == oldTemp) {
        (*item) = newTemp;
      }
    }
  }
  void Del(Temp *oldTemp) { temp_list_.remove(oldTemp); }

private:
  std::list<Temp *> temp_list_;
};

} // namespace temp

#endif