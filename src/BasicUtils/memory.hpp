#pragma once
#define NOMINMAX
#include <Windows.h>
#include <Psapi.h>
#include <vector>
#include <string>
#include <optional>
#include <cstdint>
#include <cstring>
#include <type_traits>
#include <stdexcept>
#include <cwctype>
#include <unordered_map>
#include <iterator>
#include <array>
#include <mutex>
#include <algorithm>
#include <cctype>
#include <detours.h>

namespace mem
{
    using addr_t = uintptr_t;
    using byte_t = uint8_t;
    using bytevec_t = std::vector<byte_t>;

    template <typename CharT>
    struct ModuleInfo
    {
        std::basic_string<CharT> name;
        addr_t base = 0;
        size_t size = 0;

        ModuleInfo() = default;
        ModuleInfo(const std::basic_string<CharT> &n, addr_t b, size_t s)
            : name(n), base(b), size(s) {}

        addr_t End() const noexcept { return base + size; }
        bool Contains(addr_t addr) const noexcept { return addr >= base && addr < End(); }
        explicit operator bool() const noexcept { return size != 0; }
    };

    template <typename CharT>
    class Memory
    {
    public:
        using string_t = std::basic_string<CharT>;
        using modinfo_t = ModuleInfo<CharT>;

        explicit Memory(HANDLE proc = GetCurrentProcess()) noexcept
            : m_proc(proc), m_mod({}, 0, 0) { m_mod = GetRange(); }

        Memory(const string_t &mod, HANDLE proc = GetCurrentProcess()) noexcept
            : m_proc(proc), m_mod(mod, 0, 0) { m_mod = GetRange(); }

        template <typename U, typename = std::enable_if_t<std::is_integral_v<U> || std::is_pointer_v<U>>>
        Memory(U base, size_t size, HANDLE proc = GetCurrentProcess()) noexcept
            : m_proc(proc), m_mod({}, reinterpret_cast<addr_t>(base), size) {}

        class Address
        {
        public:
            static constexpr addr_t invalid = static_cast<addr_t>(-1);

            Address() noexcept : m_memory(nullptr), m_addr(invalid), m_detour(nullptr), m_hooked(false) {}
            Address(Memory<CharT> &mem, addr_t addr, bool rva = false) noexcept
                : m_memory(&mem), m_addr(rva ? mem.m_mod.base + addr : addr), m_detour(nullptr), m_hooked(false) {}

            Address(const Address &) noexcept = default;
            Address(Address &&) noexcept = default;
            Address &operator=(const Address &) noexcept = default;
            Address &operator=(Address &&) noexcept = default;
            Address &operator=(addr_t addr) noexcept
            {
                m_addr = addr;
                return *this;
            }

            explicit operator bool() const noexcept { return IsValid(); }

            bool IsValid(const string_t &pat = {}) const noexcept
            {
                if (m_addr == invalid || !m_memory)
                    return false;
                if (pat.empty())
                    return true;
                return m_memory->VerifyMemoryPattern(m_addr, pat);
            }

            std::vector<Address> Scan(const string_t &pat, size_t size = 0) const noexcept
            {
                if (!IsValid())
                    return {};
                size_t region = size ? size : (m_memory->m_mod.base + m_memory->m_mod.size - m_addr);
                m_memory->m_mod = {{}, m_addr, region};
                return m_memory->Scan(pat, false);
            }

            Address Find(const string_t &pat, size_t size = 0) const noexcept
            {
                if (!IsValid())
                    return Address{};
                size_t region = size ? size : (m_memory->m_mod.base + m_memory->m_mod.size - m_addr);
                m_memory->m_mod = {{}, m_addr, region};
                return m_memory->Find(pat);
            }

            std::vector<Address> ScanReferences(const string_t &prefix = {}, bool rip = false) const noexcept
            {
                if (!IsValid())
                    return {};
                return m_memory->ScanReferences(m_addr, prefix, rip, false);
            }

            Address FindReference(const string_t &prefix = {}, bool rip = false) const noexcept
            {
                if (!IsValid())
                    return Address{};
                return m_memory->FindReference(m_addr, prefix, rip);
            }

            template <typename T>
            bool Read(T &out) const noexcept
            {
                if (!IsValid())
                    return false;
                return m_memory->Read(m_addr, out);
            }

            template <typename T>
            bool Write(const T &value) const noexcept
            {
                if (!IsValid())
                    return false;
                return m_memory->Write(m_addr, value);
            }

            template <typename T>
            bool Hook(T *target, T detour, const string_t &pat = {})
            {
                if (!IsValid(pat) || m_hooked)
                    return false;
                if (!m_memory->Hook(target, detour))
                    return false;
                m_addr = reinterpret_cast<addr_t>(target);
                m_hooked = true;
                m_detour = reinterpret_cast<void *>(detour);
                return true;
            }

            template <typename T>
            bool Unhook(T *target)
            {
                if (!IsValid() || !m_hooked)
                    return false;
                if (!m_memory->Unhook(target, reinterpret_cast<T>(m_detour)))
                    return false;
                m_addr = reinterpret_cast<addr_t>(target);
                m_hooked = false;
                m_detour = nullptr;
                return true;
            }

            addr_t Get() const noexcept { return m_addr; }
            addr_t RVA() const noexcept { return IsValid() ? m_addr - m_memory->m_mod.base : invalid; }

            Address Offset(ptrdiff_t offset) const noexcept
            {
                using signed_t = std::make_signed_t<addr_t>;
                signed_t r = static_cast<signed_t>(m_addr) + offset;
                return r < 0 ? Address{} : Address(*m_memory, static_cast<addr_t>(r));
            }

            template <typename... Offs>
            Address Resolve(Offs... offs) const noexcept
            {
                addr_t a = m_addr;
                for (ptrdiff_t off : {offs...})
                    if (!m_memory->Read(a + off, a) || a == invalid)
                        return {};
                return {*m_memory, a};
            }

            template <typename F, typename... A>
            auto Call(A &&...a) const noexcept
            {
                if (!IsValid())
                {
                    if constexpr (sizeof...(A) == 0)
                        return (F) nullptr;
                    else if constexpr (std::is_void_v<std::invoke_result_t<F, A...>>)
                        return;
                    else
                        return std::invoke_result_t<F, A...>{};
                }

                auto fn = reinterpret_cast<F>(m_addr);
                if constexpr (sizeof...(A) == 0)
                    return fn;
                else if constexpr (std::is_void_v<std::invoke_result_t<F, A...>>)
                    fn(std::forward<A>(a)...);
                else
                    return fn(std::forward<A>(a)...);
            }

            Address Follow() const noexcept
            {
                if (m_addr == invalid || !m_memory)
                    return {};
                const byte_t *p = reinterpret_cast<const byte_t *>(m_addr);

                auto read8 = [&](size_t off)
                { return static_cast<int8_t>(p[off]); };
                auto read32 = [&](size_t off)
                { int32_t v; std::memcpy(&v, p + off, sizeof(v)); return v; };
                auto read64 = [&](size_t off)
                { int64_t v; std::memcpy(&v, p + off, sizeof(v)); return v; };

                auto imm8 = [&](size_t off) -> Address
                { return Address{*m_memory, static_cast<addr_t>(read8(off))}; };
                auto imm32 = [&](size_t off) -> Address
                { return Address{*m_memory, static_cast<addr_t>(read32(off))}; };
                auto imm64 = [&](size_t off) -> Address
                { return Address{*m_memory, static_cast<addr_t>(read64(off))}; };
                auto rel = [&](size_t len, auto disp) -> Address
                { return Address{*m_memory, m_addr + len + disp()}; };

                byte_t op = p[0];

                // 1-byte immediate
                if (op == 0x6A)
                    return imm8(1);

                // 4-byte immediate: push/add/or/cmp/test eax, mov reg, CMP EAX, imm32
                if ((op >= 0x68 && op <= 0x6F) || (op >= 0xB8 && op <= 0xBF) ||
                    op == 0x05 || op == 0x0D || op == 0xA9 || op == 0x3D)
                    return imm32(1);

                // mov r64, imm64 (REX.W + B8+r)
                if (op == 0x48 && (p[1] & 0xF0) == 0xB8)
                    return imm64(2);

                // --- NEW: mov eax, dword ptr ds:[imm32]  (A1 moffs32)
                if (op == 0xA1)
                    return imm32(1);

                // --- NEW: mov rax, qword ptr ds:[imm64]  (REX.W + A1 moffs64)
                if (op == 0x48 && p[1] == 0xA1)
                    return imm64(2);

                // CMP r/m32, imm8 or imm32 (/7)
                if (op == 0x83 || op == 0x81)
                {
                    byte_t modrm = p[1];
                    if (((modrm >> 3) & 7) == 7)
                    {
                        int mod = modrm >> 6;
                        int rm = modrm & 7;
                        size_t disp = (mod == 1 ? 1 : (mod == 2 || (mod == 0 && rm == 5) ? 4 : 0));
                        size_t off = 2 + disp;
                        return op == 0x83 ? imm8(off) : imm32(off);
                    }
                }

                // short jmp/jcc
                if (op == 0xEB || (op >= 0x70 && op <= 0x7F))
                    return rel(2, [&]
                               { return read8(1); });

                // near jcc 0x0F 8x
                if (op == 0x0F && p[1] >= 0x80 && p[1] <= 0x8F)
                    return rel(6, [&]
                               { return read32(2); });

                // call/jmp rel32
                if (op == 0xE8 || op == 0xE9)
                    return rel(5, [&]
                               { return read32(1); });

                // CMP r/m32, r32 (/r)
                if (op == 0x39)
                {
                    byte_t modrm = p[1];
                    int mod = modrm >> 6, rm = modrm & 7;
                    if (mod == 0 && rm == 5)
                        return imm32(2);
                }

                // LEA/MOV rip-relative (8D/8B, modrm = 0x05, disp32)
                if ((op == 0x8D || op == 0x8B) && p[1] == 0x05)
                    return rel(6, [&]
                               { return read32(2); });

                return {};
            }

            bool operator==(const Address &other) const noexcept { return m_addr == other.m_addr; }
            bool operator!=(const Address &other) const noexcept { return m_addr != other.m_addr; }

        private:
            Memory *m_memory;
            addr_t m_addr;
            void *m_detour;
            bool m_hooked;
        };

        template <typename U>
            requires(std::is_integral_v<U> || std::is_pointer_v<U>)
        Address AtAddress(U addr, const string_t &pat = {}, bool rva = false) const noexcept
        {
            addr_t a;
            if constexpr (std::is_pointer_v<U>)
            {
                a = reinterpret_cast<addr_t>(addr);
            }
            else
            {
                a = static_cast<addr_t>(addr);
            }
            if (a == Address::invalid)
                return {};
            if (rva)
                a += m_mod.base;
            if (!pat.empty() && !VerifyMemoryPattern(a, pat))
                return {};
            return Address{const_cast<Memory &>(*this), a};
        }

        Address GetFunction(const char *name) const
        {
            HMODULE h;
            if constexpr (std::is_same_v<CharT, wchar_t>)
            {
                h = GetModuleHandleW(m_mod.name.empty() ? nullptr : m_mod.name.c_str());
            }
            else
            {
                h = GetModuleHandleA(m_mod.name.empty() ? nullptr : m_mod.name.c_str());
            }
            if (auto proc = h ? GetProcAddress(h, name) : nullptr)
            {
                return {const_cast<Memory &>(*this), reinterpret_cast<addr_t>(proc)};
            }
            return {};
        }

        std::vector<Address> Scan(const string_t &pat, bool first = false) const
        {
            if (m_mod.size == 0) return {};
            auto raw = ScanRange(m_mod.base, m_mod.size, Parse(pat), first);
            std::vector<Address> result;
            result.reserve(raw.size());
            for (addr_t a : raw)
                result.emplace_back(const_cast<Memory &>(*this), a);
            return result;
        }

        Address Find(const string_t &pat) const
        {
            auto v = Scan(pat, true);
            return v.empty() ? Address{} : v.front();
        }

        std::vector<Address> ScanReferences(addr_t target, const string_t &prefix = {}, bool rip = false, bool first = false) const
        {
            if (m_mod.size == 0)
                return {};
            auto [pvals, pmask] = Parse(prefix);
            size_t plen = pvals.size();
            SYSTEM_INFO si;
            GetSystemInfo(&si);
            size_t ps = si.dwPageSize;
            std::vector<addr_t> hits_all;
            bytevec_t buf;
            buf.reserve(ps);
            for (addr_t a = m_mod.base; a < m_mod.base + m_mod.size; a += ps)
            {
                size_t chunk = std::min<addr_t>(ps, m_mod.base + m_mod.size - a);
                buf.resize(chunk);
                SIZE_T rd;
                if (!ReadProcessMemory(m_proc, reinterpret_cast<LPCVOID>(a), buf.data(), chunk, &rd) || rd == 0)
                    continue;
                if (!rip)
                {
                    bytevec_t tgt(sizeof(addr_t));
                    for (size_t i = 0; i < sizeof(addr_t); i++)
                        tgt[i] = static_cast<byte_t>((target >> (8 * i)) & 0xFF);
                    bytevec_t vals(pvals), mask(pmask);
                    vals.insert(vals.end(), tgt.begin(), tgt.end());
                    mask.insert(mask.end(), sizeof(addr_t), 0xFF);
                    auto hits = ScanChunk(a, buf, rd, vals, mask, pvals.size() + sizeof(addr_t), first);
                    hits_all.insert(hits_all.end(), hits.begin(), hits.end());
                }
                else
                {
                    if (rd < plen + sizeof(int32_t))
                        continue;
                    for (size_t i = 0; i <= rd - plen - sizeof(int32_t); i++)
                    {
                        bool ok = true;
                        for (size_t j = 0; j < plen; j++)
                        {
                            if ((buf[i + j] & pmask[j]) != (pvals[j] & pmask[j]))
                            {
                                ok = false;
                                break;
                            }
                        }
                        if (!ok)
                            continue;
                        int32_t disp;
                        std::memcpy(&disp, &buf[i + plen], sizeof(disp));
                        addr_t ins = a + i;
                        if (ins + plen + sizeof(disp) + static_cast<addr_t>(disp) == target)
                        {
                            hits_all.push_back(ins);
                        }
                    }
                }
                if (first && !hits_all.empty())
                    break;
            }
            std::vector<Address> result;
            result.reserve(hits_all.size());
            for (addr_t a : hits_all)
                result.emplace_back(const_cast<Memory &>(*this), a);
            return result;
        }

        Address FindReference(addr_t target, const string_t &prefix = {}, bool rip = false) const
        {
            auto v = ScanReferences(target, prefix, rip, true);
            return v.empty() ? Address{} : v.front();
        }

        template <typename T>
        bool Read(addr_t addr, T &out, SIZE_T s = sizeof(T)) const noexcept
        {
            if (addr == Address::invalid)
                return false;
            void *b;
            SIZE_T l;
            if constexpr (std::is_same_v<T, bytevec_t>)
            {
                if (out.empty())
                    return false;
                b = out.data();
                l = out.size();
            }
            else
            {
                static_assert(std::is_trivially_copyable_v<T>);
                b = &out;
                l = s;
            }
            SIZE_T r;
            if (ReadProcessMemory(m_proc, (LPCVOID)addr, b, l, &r) && r == l)
                return true;
            memset(b, 0, l);
            return false;
        }

        template <typename T>
        bool Write(addr_t addr, const T &val) const noexcept
        {
            if (addr == Address::invalid)
                return false;
            const byte_t *p = reinterpret_cast<const byte_t *>(
                std::is_arithmetic_v<T> ? static_cast<const void *>(&val) : static_cast<const void *>(std::data(val)));
            size_t n = std::is_arithmetic_v<T> ? sizeof(val) : std::size(val);
            if (DWORD old; VirtualProtectEx(m_proc, reinterpret_cast<LPVOID>(addr), n, PAGE_EXECUTE_READWRITE, &old))
            {
                SIZE_T w;
                bool ok = WriteProcessMemory(m_proc, reinterpret_cast<LPVOID>(addr), p, n, &w) == TRUE && w == n;
                VirtualProtectEx(m_proc, reinterpret_cast<LPVOID>(addr), n, old, &old);
                return ok;
            }
            return false;
        }

        bool VerifyMemoryPattern(addr_t addr, const string_t &pat) const noexcept
        {
            if (addr == Address::invalid || pat.empty())
                return false;
            auto [vals, mask] = Parse(pat);
            bytevec_t bytes(vals.size());
            if (!Read(addr, bytes, bytes.size()))
                return false;
            for (size_t i = 0; i < vals.size(); i++)
                if ((bytes[i] & mask[i]) != (vals[i] & mask[i]))
                    return false;
            return true;
        }

        template <typename T>
        bool Hook(T *target, T detour) const noexcept
        {
            std::lock_guard<std::mutex> lock(hook_mtx);
            if (DetourIsHelperProcess())
                return false;
            if (DetourTransactionBegin() != NO_ERROR || DetourUpdateThread(GetCurrentThread()) != NO_ERROR)
                return false;
            if (DetourAttach(reinterpret_cast<void **>(target), reinterpret_cast<void *>(detour)) != NO_ERROR)
            {
                DetourTransactionAbort();
                return false;
            }
            if (DetourTransactionCommit() != NO_ERROR)
                return false;
            return true;
        }

        template <typename T>
        bool Unhook(T *target, T detour) const noexcept
        {
            std::lock_guard<std::mutex> lock(hook_mtx);
            if (DetourIsHelperProcess())
                return false;
            if (DetourTransactionBegin() != NO_ERROR || DetourUpdateThread(GetCurrentThread()) != NO_ERROR)
                return false;
            if (DetourDetach(reinterpret_cast<void **>(target), reinterpret_cast<void *>(detour)) != NO_ERROR)
            {
                DetourTransactionAbort();
                return false;
            }
            if (DetourTransactionCommit() != NO_ERROR)
                return false;
            return true;
        }

    private:
        HANDLE m_proc;
        modinfo_t m_mod;
        static std::unordered_map<string_t, modinfo_t> m_cache;
        static std::mutex hook_mtx;
        struct Parsed
        {
            bytevec_t v, m;
        };

        modinfo_t GetRange() const
        {
            auto it = m_cache.find(m_mod.name);
            if (it != m_cache.end())
                return it->second;
            modinfo_t info;
            if (!QueryModule(info))
                throw std::runtime_error("QueryModule failed");
            return m_cache[m_mod.name] = info;
        };

        bool QueryModule(modinfo_t& out) const noexcept {
            out = {};
            HMODULE target = nullptr;

            if (m_proc == GetCurrentProcess()) {
                if (m_mod.name.empty()) target = GetModuleHandle(nullptr);
                else if constexpr (std::is_same_v<CharT, wchar_t>) target = GetModuleHandleW(m_mod.name.c_str());
                else target = GetModuleHandleA(m_mod.name.c_str());
            }

            if (!target) {
                DWORD need = 0;
                if (!EnumProcessModules(m_proc, nullptr, 0, &need) || !need) return false;
                std::vector<HMODULE> mods(need / sizeof(HMODULE));
                if (!EnumProcessModules(m_proc, mods.data(), static_cast<DWORD>(mods.size() * sizeof(HMODULE)), &need)) return false;
                mods.resize(need / sizeof(HMODULE));
                if (mods.empty()) return false;

                if (m_mod.name.empty()) target = mods.front();
                else {
                    std::array<CharT, MAX_PATH> buf{};
                    for (HMODULE h : mods) {
                        if constexpr (std::is_same_v<CharT, wchar_t>) {
                            if (!GetModuleBaseNameW(m_proc, h, buf.data(), (DWORD)buf.size())) continue;
                            if (_wcsicmp(buf.data(), m_mod.name.c_str()) == 0) { target = h; break; }
                        }
                        else {
                            if (!GetModuleBaseNameA(m_proc, h, buf.data(), (DWORD)buf.size())) continue;
                            if (_stricmp(buf.data(), m_mod.name.c_str()) == 0) { target = h; break; }
                        }
                    }
                }
            }

            if (!target) return false;
            MODULEINFO mi{};
            if (!GetModuleInformation(m_proc, target, &mi, sizeof(mi))) return false;

            out = {
                m_mod.name.empty() ? string_t() : m_mod.name,
                static_cast<addr_t>(reinterpret_cast<std::uintptr_t>(mi.lpBaseOfDll)),
                static_cast<std::size_t>(mi.SizeOfImage)
            };
            return true;
        }

        Parsed Parse(const string_t &s) const
        {
            auto isHexChar = [](CharT c)
            {
                if constexpr (std::is_same_v<CharT, wchar_t>)
                    return std::iswxdigit(c) || c == L'?' || std::iswspace(c);
                else
                    return std::isxdigit(c) || c == '?' || std::isspace(c);
            };
            if (!s.empty() && std::all_of(s.begin(), s.end(), isHexChar))
                return ParseHex(s);
            auto bytes = TextToBytes(s);
            return Parsed{bytes, bytevec_t(bytes.size(), 0xFF)};
        }

        Parsed ParseHex(const string_t &s) const
        {
            string_t tmp;
            tmp.reserve(s.size());
            for (auto c : s)
                if (!std::isspace(c))
                    tmp.push_back(c);
            if (tmp.size() % 2)
                throw std::invalid_argument("Invalid hex pattern");
            size_t n = tmp.size() / 2;
            bytevec_t vals(n), mask(n);
            auto decode = [&](CharT c)
            {
                if (c == '?')
                    return std::pair<byte_t, byte_t>{0, 0};
                char cc = static_cast<char>(c);
                byte_t v = std::isdigit(cc) ? cc - '0' : std::toupper(cc) - 'A' + 10;
                return std::pair<byte_t, byte_t>{v, 0xF};
            };
            for (size_t i = 0; i < n; i++)
            {
                auto [hi, hm] = decode(tmp[2 * i]);
                auto [lo, lm] = decode(tmp[2 * i + 1]);
                vals[i] = (hi << 4) | lo;
                mask[i] = (hm << 4) | lm;
            }
            return {vals, mask};
        }

        bytevec_t TextToBytes(const string_t &s) const
        {
            bytevec_t out;
            if constexpr (std::is_same_v<CharT, wchar_t>)
            {
                bool ascii = std::all_of(s.begin(), s.end(), [](wchar_t c)
                                         { return !(c & 0xFF00); });
                out.reserve(s.size() * (ascii ? 1 : 2));
                for (wchar_t w : s)
                {
                    out.push_back(byte_t(w & 0xFF));
                    if (!ascii)
                        out.push_back(byte_t((w >> 8) & 0xFF));
                }
            }
            else
            {
                out.assign(s.begin(), s.end());
            }
            return out;
        }

        std::vector<addr_t> ScanRange(addr_t base, size_t size, const Parsed &p, bool first) const
        {
            std::vector<addr_t> out;
            const size_t plen = p.v.size();
            if (size < plen)
                return out;
            const size_t chunkSz = 64 * 1024;
            addr_t end = base + size;
            for (addr_t pos = base; pos < end; pos += chunkSz)
            {
                size_t len = std::min(chunkSz + plen - 1, end - pos);
                bytevec_t buf(len);
                SIZE_T rd = 0;
                if (ReadProcessMemory(m_proc, (LPCVOID)pos, buf.data(), len, &rd) && rd >= plen)
                {
                    for (auto addr : ScanChunk(pos, buf, rd, p.v, p.m, plen, first))
                    {
                        out.push_back(addr);
                        if (first)
                            return out;
                    }
                }
            }
            return out;
        }

        std::vector<addr_t> ScanChunk(addr_t base,
                                      const bytevec_t &buf, SIZE_T rd, const bytevec_t &vals,
                                      const bytevec_t &mask, size_t len, bool first) const
        {
            std::vector<addr_t> out;
            if (rd < len)
                return out;
            const byte_t *data = buf.data();
            size_t maxOff = rd - len;
            const byte_t key = vals[0];
            size_t off = 0;
            while (off <= maxOff)
            {
                auto hit = (const byte_t *)std::memchr(data + off, key, maxOff - off + 1);
                if (!hit)
                    break;
                size_t idx = hit - data;
                bool ok = true;
                for (size_t j = 1; j < len; ++j)
                    if ((data[idx + j] & mask[j]) != (vals[j] & mask[j]))
                    {
                        ok = false;
                        break;
                    }
                if (ok)
                {
                    out.push_back(base + idx);
                    if (first)
                        break;
                }
                off = idx + 1;
            }
            return out;
        }
    };

    template <typename CharT>
    std::unordered_map<std::basic_string<CharT>, ModuleInfo<CharT>> Memory<CharT>::m_cache;

    template <typename CharT>
    std::mutex Memory<CharT>::hook_mtx;

} // namespace mem

using MemoryA = mem::Memory<char>;
using MemoryW = mem::Memory<wchar_t>;
