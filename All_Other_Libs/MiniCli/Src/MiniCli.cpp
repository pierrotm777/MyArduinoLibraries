#include "MiniCli.h"
#include <string.h>
#include <stdlib.h>

// ─────────────────────────────────────────────
// Ctor / init
// ─────────────────────────────────────────────
MiniCli::MiniCli() { _buf[0] = 0; }

void MiniCli::begin(Stream& in, Stream& out) {
  _in = &in;
  _out = &out;
  _n = 0;
  _buf[0] = 0;
}

void MiniCli::setPreDispatch(PreDispatchFn fn) { _pre = fn; }
void MiniCli::setGlobals(const GlobalCmd* cmds, uint8_t count) { _globals = cmds; _globalsCount = count; }
void MiniCli::setObjects(const ObjectSpec* objs, uint8_t count) { _objs = objs; _objsCount = count; }

// ─────────────────────────────────────────────
// Utils (tokens)
// ─────────────────────────────────────────────
bool MiniCli::isSpace(char c) { return (c==' ' || c=='\t' || c=='\r' || c=='\n'); }

void MiniCli::trimLeft(const char*& p) { while (*p && isSpace(*p)) p++; }

void MiniCli::trimRightInPlace(char* s) {
  if (!s) return;
  size_t n = strlen(s);
  while (n > 0 && isSpace(s[n - 1])) { s[n - 1] = 0; n--; }
}

bool MiniCli::streqi(const char* a, const char* b) {
  if (!a || !b) return false;
  while (*a && *b) {
    char ca = *a++, cb = *b++;
    if (ca >= 'a' && ca <= 'z') ca = (char)(ca - 'a' + 'A');
    if (cb >= 'a' && cb <= 'z') cb = (char)(cb - 'a' + 'A');
    if (ca != cb) return false;
  }
  return (*a == 0 && *b == 0);
}

bool MiniCli::startsWithI(const char* s, const char* pfx) {
  if (!s || !pfx) return false;
  while (*pfx) {
    char cs = *s++, cp = *pfx++;
    if (cs >= 'a' && cs <= 'z') cs = (char)(cs - 'a' + 'A');
    if (cp >= 'a' && cp <= 'z') cp = (char)(cp - 'a' + 'A');
    if (cs != cp) return false;
  }
  return true;
}

bool MiniCli::nextToken(const char*& p, char* tok, size_t tsz) {
  trimLeft(p);
  if (!*p) return false;
  size_t n = 0;
  while (*p && !isSpace(*p)) {
    if (n + 1 < tsz) tok[n++] = *p;
    p++;
  }
  tok[n] = 0;
  return true;
}

bool MiniCli::parseKeyVal(const char* tok, char* key, size_t ksz, char* val, size_t vsz) {
  const char* eq = strchr(tok, '=');
  if (!eq) return false;

  size_t kl = (size_t)(eq - tok);
  size_t vl = strlen(eq + 1);

  if (kl >= ksz) kl = ksz - 1;
  if (vl >= vsz) vl = vsz - 1;

  memcpy(key, tok, kl); key[kl] = 0;
  memcpy(val, eq + 1, vl); val[vl] = 0;
  return true;
}

// ─────────────────────────────────────────────
// tick
// ─────────────────────────────────────────────
void MiniCli::tick() {
  if (!_in || !_out) return;

  while (_in->available()) {
    char c = (char)_in->read();
    if (c == '\r') continue;

    if (c == '\n') {
      _buf[_n] = 0;
      trimRightInPlace(_buf);
      if (_n > 0) handleLine(_buf);
      _n = 0;
      _buf[0] = 0;
    } else {
      if (_n + 1 < BUF_SZ) _buf[_n++] = c;
      // overflow silencieux
    }
  }
}

void MiniCli::handleLine(const char* line) {
  if (!_out || !line) return;

  while (*line == ' ') line++;
  if (!*line) return;

  if (_pre) {
    if (_pre(line, *_out)) return;
  }

  if (tryGlobal(line)) return;
  if (tryObject(line)) return;

  _out->println(F("ERR unknown command"));
}

// ─────────────────────────────────────────────
// Globals
// ─────────────────────────────────────────────
bool MiniCli::tryGlobal(const char* line) {
  if (!_globals || _globalsCount == 0) return false;

  const char* p = line;
  char head[24];
  if (!nextToken(p, head, sizeof(head))) return false;

  for (uint8_t i = 0; i < _globalsCount; i++) {
    if (streqi(head, _globals[i].name)) {
      return _globals[i].fn(*_out);
    }
  }
  return false;
}

// ─────────────────────────────────────────────
// Objects lookup / index parsing
// ─────────────────────────────────────────────
const MiniCli::ObjectSpec* MiniCli::findObjectSpec(const char* head) const {
  if (!_objs || _objsCount == 0) return nullptr;
  for (uint8_t i = 0; i < _objsCount; i++) {
    const ObjectSpec& s = _objs[i];
    if (!s.indexed) {
      if (streqi(head, s.name)) return &s;
    } else {
      if (startsWithI(head, s.name)) return &s; // SERVO2, M1
    }
  }
  return nullptr;
}

bool MiniCli::parseObjectHead(const ObjectSpec* spec, const char* head, uint8_t& idx0) const {
  idx0 = 0;
  if (!spec) return false;

  if (!spec->indexed) {
    idx0 = 0;
    return true;
  }

  const char* p = head + strlen(spec->name);
  if (!*p) return false;

  uint32_t n = 0;
  while (*p >= '0' && *p <= '9') {
    n = n * 10 + (uint32_t)(*p - '0');
    p++;
  }
  if (*p != 0) return false;

  if (n < spec->idxMin1 || n > spec->idxMax1) return false;
  idx0 = (uint8_t)(n - 1);
  return true;
}

// ─────────────────────────────────────────────
// Object dispatch (support "SYS?" collé)
// ─────────────────────────────────────────────
bool MiniCli::tryObject(const char* line) {
  const char* p = line;
  char head[24];
  if (!nextToken(p, head, sizeof(head))) return false;

  bool inlineQuery = false;
  size_t hl = strlen(head);
  if (hl >= 2 && head[hl - 1] == '?') {
    inlineQuery = true;
    head[hl - 1] = 0;
  }

  const ObjectSpec* spec = findObjectSpec(head);
  if (!spec) return false;

  uint8_t idx0 = 0;
  if (!parseObjectHead(spec, head, idx0)) {
    _out->println(F("ERR bad object index"));
    return true;
  }

  trimLeft(p);

  if (inlineQuery) {
    if (p && *p) {
      _out->println(F("ERR: '?' must be last"));
      return true;
    }
    execObject(spec, idx0, "?");
    return true;
  }

  execObject(spec, idx0, p);
  return true;
}

// ─────────────────────────────────────────────
// Parse helpers (exposés)
// ─────────────────────────────────────────────
bool MiniCli::parseU32(const char* s, uint32_t& out) {
  if (!s || !*s) return false;
  char* end = nullptr;
  unsigned long v = strtoul(s, &end, 10);
  if (end == s) return false;
  out = (uint32_t)v;
  return true;
}

bool MiniCli::parseBool01(const char* s, bool& out) {
  uint32_t v = 0;
  if (!parseU32(s, v) || v > 1) return false;
  out = (v != 0);
  return true;
}

bool MiniCli::parseU16Range(const char* s, uint16_t& out,
                            uint16_t minv, uint16_t maxv,
                            Stream& outS, const __FlashStringHelper* err) {
  uint32_t v = 0;
  if (!parseU32(s, v) || v < minv || v > maxv) {
    outS.println(err);
    return false;
  }
  out = (uint16_t)v;
  return true;
}

bool MiniCli::parseU8Max(const char* s, uint8_t& out,
                         uint8_t maxv,
                         Stream& outS, const __FlashStringHelper* err) {
  uint32_t v = 0;
  if (!parseU32(s, v) || v > maxv) {
    outS.println(err);
    return false;
  }
  out = (uint8_t)v;
  return true;
}

bool MiniCli::parseBool01OrErr(const char* s, bool& out,
                               Stream& outS, const __FlashStringHelper* err) {
  if (!parseBool01(s, out)) {
    outS.println(err);
    return false;
  }
  return true;
}

bool MiniCli::parseEnumU32(const char* s,
                           const EnumItem* items, uint8_t count,
                           uint32_t& out) {
  if (!s || !*s || !items || count == 0) return false;

  uint32_t v = 0;
  if (parseU32(s, v)) { out = v; return true; }

  for (uint8_t i = 0; i < count; i++) {
    if (items[i].name && streqi(s, items[i].name)) {
      out = items[i].value;
      return true;
    }
  }
  return false;
}

void MiniCli::printEnumList(const EnumItem* items, uint8_t count, Stream& outS) {
  if (!items || count == 0) return;
  for (uint8_t i = 0; i < count; i++) {
    outS.print(items[i].name ? items[i].name : "?");
    outS.print('=');
    outS.print(items[i].value);
    if (i + 1 < count) outS.print(F(" | "));
  }
}

bool MiniCli::parseEnumU32OrErr(const char* s,
                                const EnumItem* items, uint8_t count,
                                uint32_t& out,
                                Stream& outS, const __FlashStringHelper* errPrefix) {
  if (parseEnumU32(s, items, count, out)) return true;

  if (errPrefix) outS.print(errPrefix);
  outS.print(F(" (expected: "));
  printEnumList(items, count, outS);
  outS.println(F(")"));
  return false;
}

bool MiniCli::parseMxCy(const char* s,
                        uint8_t mMin, uint8_t mMax,
                        uint8_t cMin, uint8_t cMax,
                        uint8_t& mIdx0,
                        uint8_t& c1based) {
  if (!s || !*s) return false;

  if (s[0] != 'M' && s[0] != 'm') return false;
  s++;

  const char* colon = strchr(s, ':');
  if (!colon) return false;

  char mbuf[8];
  size_t ml = (size_t)(colon - s);
  if (ml == 0 || ml >= sizeof(mbuf)) return false;
  memcpy(mbuf, s, ml);
  mbuf[ml] = 0;

  uint32_t m = 0;
  if (!parseU32(mbuf, m)) return false;

  const char* p = colon + 1;
  if (*p != 'C' && *p != 'c') return false;
  p++;

  uint32_t c = 0;
  if (!parseU32(p, c)) return false;

  if (m < mMin || m > mMax) return false;
  if (c < cMin || c > cMax) return false;

  mIdx0 = (uint8_t)(m - 1);
  c1based = (uint8_t)c;
  return true;
}

bool MiniCli::parsePrefixU8(const char* s,
                            const char* prefix,
                            uint8_t minv, uint8_t maxv,
                            uint8_t& out) {
  if (!s || !*s || !prefix || !*prefix) return false;
  if (!startsWithI(s, prefix)) return false;

  uint32_t v = 0;
  if (!parseU32(s + strlen(prefix), v)) return false;
  if (v < minv || v > maxv) return false;

  out = (uint8_t)v;
  return true;
}

// ─────────────────────────────────────────────
// KEY indexée : "P1" ou "P(1)" -> base="P", idx1=1
// ─────────────────────────────────────────────
static bool splitKeyIndexed(const char* keyIn, char* base, size_t bsz, uint8_t& idx1) {
  idx1 = 0;
  if (!keyIn || !*keyIn) return false;

  const char* p = keyIn;
  size_t n = 0;

  // base: lettres + underscore
  while (*p && (((*p >= 'A' && *p <= 'Z') || (*p >= 'a' && *p <= 'z')) || *p == '_')) {
    if (n + 1 < bsz) base[n++] = *p;
    p++;
  }
  base[n] = 0;
  if (n == 0) return false;

  // Forme P<digits>
  if (*p >= '0' && *p <= '9') {
    uint32_t v = 0;
    if (!MiniCli::parseU32(p, v)) return false;
    if (v == 0 || v > 255) return false;
    idx1 = (uint8_t)v;
    return true;
  }

  // Forme P(<digits>)
  if (*p == '(') {
    p++;
    uint32_t v = 0;
    if (!MiniCli::parseU32(p, v)) return false;
    while (*p && (*p >= '0' && *p <= '9')) p++;
    if (*p != ')') return false;
    p++;
    if (*p != 0) return false;
    if (v == 0 || v > 255) return false;
    idx1 = (uint8_t)v;
    return true;
  }

  return false; // non indexé
}

// ─────────────────────────────────────────────
// Execute object (Query / SUBOBJ / KEY=VAL / KEY<n>=VAL)
// ─────────────────────────────────────────────
void MiniCli::execObject(const ObjectSpec* spec, uint8_t idx0, const char* rest) {
  if (!spec) return;

  // Query
  if (!rest || !*rest || (rest[0] == '?' && rest[1] == 0)) {
    if (spec->onQuery) spec->onQuery(idx0, *_out);
    else _out->println(F("ERR no query handler"));
    return;
  }

  const char* p = rest;
  trimLeft(p);

  // Sélection fields: direct OBJ ou SUBOBJ
  const Field* fields = spec->fields;
  uint8_t fieldCount = spec->fieldCount;
  uint8_t maxKv = spec->maxKvPerLine;

  // SUBOBJ optionnel:
  // si premier token n'a pas '=', alors on tente de le matcher à un SUBOBJ
  {
    const char* ppeek = p;
    char t0[32];
    if (nextToken(ppeek, t0, sizeof(t0))) {

      // "OBJ ?" avec espace
      if (t0[0] == '?' && t0[1] == 0) {
        if (spec->onQuery) spec->onQuery(idx0, *_out);
        else _out->println(F("ERR no query handler"));
        return;
      }

      if (!strchr(t0, '=')) {
        bool subFound = false;
        for (uint8_t i = 0; i < spec->subCount; i++) {
          if (streqi(t0, spec->subs[i].name)) {
            subFound = true;
            fields = spec->subs[i].fields;
            fieldCount = spec->subs[i].fieldCount;
            maxKv = spec->subs[i].maxKvPerLine;

            // consomme SUBOBJ
            char dummy[32];
            nextToken(p, dummy, sizeof(dummy));
            trimLeft(p);
            break;
          }
        }

        // Si on n'a ni '=', ni SUBOBJ connu : erreur explicite
        if (!subFound) {
          _out->println(F("ERR expected KEY=VAL or known SUBOBJ"));
          return;
        }
      }
    }
  }

  // Count KV tokens
  uint8_t kvCount = 0;
  {
    const char* p2 = p;
    char tok[96];
    while (nextToken(p2, tok, sizeof(tok))) {
      char key[48], val[48];
      if (parseKeyVal(tok, key, sizeof(key), val, sizeof(val))) kvCount++;
      else {
        _out->println(F("ERR expected KEY=VAL"));
        return;
      }
    }
  }

  if (kvCount == 0) {
    _out->println(F("ERR expected KEY=VAL"));
    return;
  }
  if (kvCount > maxKv) {
    _out->println(F("ERR: too many options on one line"));
    return;
  }

  bool changedAny = false;

  // Apply KV
  char tok[96];
  while (nextToken(p, tok, sizeof(tok))) {
    char keyRaw[48], val[48];
    if (!parseKeyVal(tok, keyRaw, sizeof(keyRaw), val, sizeof(val))) {
      _out->println(F("ERR expected KEY=VAL"));
      return;
    }

    char baseKey[32];
    uint8_t keyIdx1 = 0;
    bool isIndexedKey = splitKeyIndexed(keyRaw, baseKey, sizeof(baseKey), keyIdx1);

    bool found = false;

    for (uint8_t i = 0; i < fieldCount; i++) {
      const Field& f = fields[i];

      const char* matchKey = isIndexedKey ? baseKey : keyRaw;
      if (!streqi(matchKey, f.key)) continue;

      found = true;
      bool changed = false;

      if (isIndexedKey) {
        if (!f.setEx || f.keyMin1 == 0 || f.keyMax1 == 0) {
          _out->println(F("ERR key not indexable"));
          return;
        }
        if (keyIdx1 < f.keyMin1 || keyIdx1 > f.keyMax1) {
          _out->println(F("ERR key index out of range"));
          return;
        }
        uint8_t keyIdx0 = (uint8_t)(keyIdx1 - 1);
        if (!f.setEx(idx0, keyIdx0, val, *_out, changed)) return;
      } else {
        if (!f.set) {
          _out->println(F("ERR field has no setter"));
          return;
        }
        if (!f.set(idx0, val, *_out, changed)) return;
      }

      if (changed) changedAny = true;
      break;
    }

    if (!found) {
      _out->print(F("ERR unknown key: "));
      _out->println(keyRaw);
      return;
    }
  }

  // Post apply (comportement identique à ta base : seulement si changedAny)
  if (changedAny && spec->postApply) {
    spec->postApply(idx0, *_out);
  }

  // Re-print current state (toujours, comme ta base)
  if (spec->onQuery) spec->onQuery(idx0, *_out);
}
