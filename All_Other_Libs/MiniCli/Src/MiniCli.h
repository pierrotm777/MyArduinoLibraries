#pragma once
#include <stdint.h>
#include <stddef.h>

#ifdef ARDUINO
  #include <Arduino.h>   // Stream, __FlashStringHelper, F()
#else
  class Stream;
  class __FlashStringHelper;
#endif

class MiniCli {
public:
  // ─────────────────────────────────────────────
  // Types publics
  // ─────────────────────────────────────────────
  using PreDispatchFn = bool (*)(const char* line, Stream& out);

  using GlobalFn = bool (*)(Stream& out);
  struct GlobalCmd {
    const char* name;
    GlobalFn    fn;
  };

  // Setter simple (non indexé)
  using FieldSetter = bool (*)(uint8_t objIdx0, const char* val, Stream& out, bool& changed);

  // Setter indexé (KEY<n>=VAL)
  using FieldSetterEx = bool (*)(uint8_t objIdx0, uint8_t keyIdx0, const char* val, Stream& out, bool& changed);

  struct Field {
    const char*   key;       // base key, ex: "MODE" ou "P"
    FieldSetter   set;       // non indexé (peut être nullptr)
    FieldSetterEx setEx;     // indexé (peut être nullptr)
    uint8_t       keyMin1;   // 0 => non indexé, sinon min (ex: 1)
    uint8_t       keyMax1;   // 0 => non indexé, sinon max (ex: 16)
  };

  using QueryFn     = void (*)(uint8_t objIdx0, Stream& out);
  using PostApplyFn = void (*)(uint8_t objIdx0, Stream& out);

  struct SubSpec {
    const char*  name;        // ex: "MAP"
    const Field* fields;      // champs dispo sous ce sous-objet
    uint8_t      fieldCount;
    uint8_t      maxKvPerLine;
  };

  struct ObjectSpec {
    const char*    name;
    bool           indexed;
    uint8_t        idxMin1;
    uint8_t        idxMax1;
    uint8_t        maxKvPerLine;   // si pas de SUBOBJ
    QueryFn        onQuery;
    PostApplyFn    postApply;

    // champs directs OBJ (sans sous-objet)
    const Field*   fields;
    uint8_t        fieldCount;

    // sous-objets optionnels
    const SubSpec* subs;
    uint8_t        subCount;
  };

  struct EnumItem {
    const char* name;
    uint32_t    value;
  };

  // ─────────────────────────────────────────────
  // API
  // ─────────────────────────────────────────────
  MiniCli();

  void begin(Stream& in, Stream& out);

  void setPreDispatch(PreDispatchFn fn);
  void setGlobals(const GlobalCmd* cmds, uint8_t count);
  void setObjects(const ObjectSpec* objs, uint8_t count);

  void tick();
  void handleLine(const char* line);

  // ─────────────────────────────────────────────
  // Utils parse (helpers)
  // ─────────────────────────────────────────────
  static bool parseU32(const char* s, uint32_t& out);
  static bool parseBool01(const char* s, bool& out);

  static bool parseU16Range(const char* s, uint16_t& out,
                            uint16_t minv, uint16_t maxv,
                            Stream& outS, const __FlashStringHelper* err);

  static bool parseU8Max(const char* s, uint8_t& out,
                         uint8_t maxv,
                         Stream& outS, const __FlashStringHelper* err);

  static bool parseBool01OrErr(const char* s, bool& out,
                               Stream& outS, const __FlashStringHelper* err);

  static bool parseEnumU32(const char* s,
                           const EnumItem* items, uint8_t count,
                           uint32_t& out);

  static void printEnumList(const EnumItem* items, uint8_t count, Stream& outS);

  static bool parseEnumU32OrErr(const char* s,
                                const EnumItem* items, uint8_t count,
                                uint32_t& out,
                                Stream& outS, const __FlashStringHelper* errPrefix);

  static bool parseMxCy(const char* s,
                        uint8_t mMin, uint8_t mMax,
                        uint8_t cMin, uint8_t cMax,
                        uint8_t& mIdx0,
                        uint8_t& c1based);

  static bool parsePrefixU8(const char* s,
                            const char* prefix,
                            uint8_t minv, uint8_t maxv,
                            uint8_t& out);

private:
  static constexpr size_t BUF_SZ = 160;

  Stream* _in  = nullptr;
  Stream* _out = nullptr;

  PreDispatchFn     _pre          = nullptr;
  const GlobalCmd*  _globals      = nullptr;
  uint8_t           _globalsCount = 0;

  const ObjectSpec* _objs      = nullptr;
  uint8_t           _objsCount = 0;

  char   _buf[BUF_SZ];
  size_t _n = 0;

  // token utils
  static bool isSpace(char c);
  static void trimLeft(const char*& p);
  static void trimRightInPlace(char* s);
  static bool streqi(const char* a, const char* b);
  static bool startsWithI(const char* s, const char* pfx);
  static bool nextToken(const char*& p, char* tok, size_t tsz);
  static bool parseKeyVal(const char* tok, char* key, size_t ksz, char* val, size_t vsz);

  // dispatch
  bool tryGlobal(const char* line);
  bool tryObject(const char* line);

  const ObjectSpec* findObjectSpec(const char* head) const;
  bool parseObjectHead(const ObjectSpec* spec, const char* head, uint8_t& idx0) const;

  void execObject(const ObjectSpec* spec, uint8_t idx0, const char* rest);
};
