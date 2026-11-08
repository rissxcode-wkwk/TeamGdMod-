#include "Db.hpp"

#include <Geode/Geode.hpp>

#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <map>
#include <sstream>

using namespace geode::prelude;
namespace fs = std::filesystem;

namespace tgm::db {
    namespace {
        constexpr char const* KEPALA = "TGDB1";
        constexpr char const* PENANDA_SUM = "#sum=";

        std::map<std::string, std::string> g_data;
        bool g_kotor = false;
        int  g_tahan = 0;
        bool g_dimuat = false;
        int  g_kondisi = 0;       // 0 = baru, 1 = normal, 2 = dipulihkan dari cadangan, 3 = gagal menulis
        std::string g_pesanGalat;

        fs::path folder() {
            // Geode 6: getSaveDir() bertipe Path milik asp, diubah lewat native()
            return fs::path(Mod::get()->getSaveDir().native()) / "database";
        }
        fs::path fileUtama()   { return folder() / "teamgd.tgdb"; }
        fs::path fileSementara() { return folder() / "teamgd.tgdb.tmp"; }
        fs::path fileCadangan() { return folder() / "teamgd.tgdb.bak"; }
        fs::path fileManual()  { return folder() / "teamgd.tgdb.manual"; }

        // FNV-1a 64-bit: cukup untuk mendeteksi file terpotong / rusak
        uint64_t fnv(std::string const& s) {
            uint64_t h = 1469598103934665603ULL;
            for (unsigned char c : s) {
                h ^= c;
                h *= 1099511628211ULL;
            }
            return h;
        }

        std::string hex(uint64_t v) {
            char buf[32];
            std::snprintf(buf, sizeof(buf), "%016llx", static_cast<unsigned long long>(v));
            return buf;
        }

        std::string escape(std::string const& s) {
            std::string o;
            o.reserve(s.size());
            for (char c : s) {
                switch (c) {
                    case '\\': o += "\\\\"; break;
                    case '\n': o += "\\n";  break;
                    case '\r': o += "\\r";  break;
                    case '\t': o += "\\t";  break;
                    default:   o += c;      break;
                }
            }
            return o;
        }

        std::string unescape(std::string const& s) {
            std::string o;
            o.reserve(s.size());
            for (std::size_t i = 0; i < s.size(); ++i) {
                if (s[i] == '\\' && i + 1 < s.size()) {
                    char n = s[++i];
                    switch (n) {
                        case 'n': o += '\n'; break;
                        case 'r': o += '\r'; break;
                        case 't': o += '\t'; break;
                        default:  o += n;    break;
                    }
                } else {
                    o += s[i];
                }
            }
            return o;
        }

        // Susun isi file lengkap (kepala + data + checksum)
        std::string susunIsi() {
            std::string badan = std::string(KEPALA) + "\n";
            for (auto const& [k, v] : g_data) {
                badan += k;
                badan += '\t';
                badan += escape(v);
                badan += '\n';
            }
            return badan + PENANDA_SUM + hex(fnv(badan)) + "\n";
        }

        bool bacaFile(fs::path const& p, std::string& isi) {
            std::error_code ec;
            if (!fs::exists(p, ec)) return false;
            std::ifstream i(p, std::ios::binary);
            if (!i) return false;
            std::ostringstream ss;
            ss << i.rdbuf();
            isi = ss.str();
            return true;
        }

        // Urai isi file. Mengembalikan false bila kepala / checksum tidak cocok.
        bool urai(std::string const& isi, std::map<std::string, std::string>& hasil) {
            auto posSum = isi.rfind(std::string("\n") + PENANDA_SUM);
            if (posSum == std::string::npos) return false;

            std::string badan = isi.substr(0, posSum + 1);
            auto sumTeks = isi.substr(posSum + 1 + std::char_traits<char>::length(PENANDA_SUM));
            while (!sumTeks.empty() && (sumTeks.back() == '\n' || sumTeks.back() == '\r')) sumTeks.pop_back();
            if (sumTeks != hex(fnv(badan))) return false;

            std::istringstream ss(badan);
            std::string baris;
            if (!std::getline(ss, baris) || baris != KEPALA) return false;
            while (std::getline(ss, baris)) {
                if (baris.empty()) continue;
                auto tab = baris.find('\t');
                if (tab == std::string::npos) continue;
                hasil[baris.substr(0, tab)] = unescape(baris.substr(tab + 1));
            }
            return true;
        }

        bool tulisKeDisk() {
            std::error_code ec;
            fs::create_directories(folder(), ec);

            auto isi = susunIsi();
            {
                std::ofstream o(fileSementara(), std::ios::binary | std::ios::trunc);
                if (!o) { g_pesanGalat = "tidak bisa membuat file sementara"; return false; }
                o.write(isi.data(), static_cast<std::streamsize>(isi.size()));
                o.flush();
                if (!o) { g_pesanGalat = "gagal menulis file"; return false; }
            }

            // Versi lama jadi cadangan, lalu file baru menggantikan file utama
            if (fs::exists(fileUtama(), ec)) {
                fs::rename(fileUtama(), fileCadangan(), ec);
                ec.clear();
            }
            fs::rename(fileSementara(), fileUtama(), ec);
            if (ec) {
                // Cadangan bila rename tidak didukung: salin langsung
                fs::copy_file(fileSementara(), fileUtama(), fs::copy_options::overwrite_existing, ec);
                if (ec) { g_pesanGalat = "gagal mengganti file utama"; return false; }
            }
            return true;
        }

        void tulisBilaPerlu(bool segera) {
            g_kotor = true;
            if (segera && g_tahan == 0) flush();
        }
    }

    void muat() {
        g_data.clear();
        g_kotor = false;
        g_kondisi = 0;
        g_pesanGalat.clear();

        std::string isi;
        std::map<std::string, std::string> hasil;

        if (bacaFile(fileUtama(), isi) && urai(isi, hasil)) {
            g_data = std::move(hasil);
            g_kondisi = 1;
        } else if (bacaFile(fileSementara(), isi) && (hasil.clear(), urai(isi, hasil))) {
            // Game mati tepat setelah menulis file sementara tetapi sebelum rename
            g_data = std::move(hasil);
            g_kondisi = 2;
            g_kotor = true;
        } else if (bacaFile(fileCadangan(), isi) && (hasil.clear(), urai(isi, hasil))) {
            g_data = std::move(hasil);
            g_kondisi = 2;
            g_kotor = true;
            log::warn("TeamGdMod: database utama rusak, dipulihkan dari cadangan");
        }

        g_dimuat = true;
        if (g_kotor) flush();
    }

    bool ada(std::string const& kunci) { return g_data.contains(kunci); }

    std::string ambil(std::string const& kunci, std::string const& bawaan) {
        auto it = g_data.find(kunci);
        return it == g_data.end() ? bawaan : it->second;
    }

    int64_t ambilInt(std::string const& kunci, int64_t bawaan) {
        auto it = g_data.find(kunci);
        if (it == g_data.end()) return bawaan;
        char* akhir = nullptr;
        auto v = std::strtoll(it->second.c_str(), &akhir, 10);
        return (akhir && akhir != it->second.c_str()) ? static_cast<int64_t>(v) : bawaan;
    }

    double ambilDbl(std::string const& kunci, double bawaan) {
        auto it = g_data.find(kunci);
        if (it == g_data.end()) return bawaan;
        char* akhir = nullptr;
        auto v = std::strtod(it->second.c_str(), &akhir);
        return (akhir && akhir != it->second.c_str()) ? v : bawaan;
    }

    bool ambilBool(std::string const& kunci, bool bawaan) {
        auto it = g_data.find(kunci);
        if (it == g_data.end()) return bawaan;
        return it->second == "1";
    }

    void set(std::string const& kunci, std::string const& nilai, bool segera) {
        auto it = g_data.find(kunci);
        if (it != g_data.end() && it->second == nilai) return;   // tidak ada perubahan
        g_data[kunci] = nilai;
        tulisBilaPerlu(segera);
    }

    void setInt(std::string const& kunci, int64_t nilai, bool segera) {
        set(kunci, std::to_string(nilai), segera);
    }

    void setDbl(std::string const& kunci, double nilai, bool segera) {
        char buf[64];
        std::snprintf(buf, sizeof(buf), "%.6f", nilai);
        set(kunci, buf, segera);
    }

    void setBool(std::string const& kunci, bool nilai, bool segera) {
        set(kunci, nilai ? "1" : "0", segera);
    }

    void tambahInt(std::string const& kunci, int64_t delta) {
        if (delta == 0) return;
        setInt(kunci, ambilInt(kunci, 0) + delta, false);
    }

    void hapus(std::string const& kunci) {
        if (g_data.erase(kunci) > 0) tulisBilaPerlu(true);
    }

    void hapusAwalan(std::string const& awalan) {
        bool ada = false;
        for (auto it = g_data.begin(); it != g_data.end();) {
            if (it->first.compare(0, awalan.size(), awalan) == 0) {
                it = g_data.erase(it);
                ada = true;
            } else {
                ++it;
            }
        }
        if (ada) tulisBilaPerlu(true);
    }

    void tahan() { ++g_tahan; }

    void lepas() {
        if (g_tahan > 0) --g_tahan;
        if (g_tahan == 0 && g_kotor) flush();
    }

    void flush() {
        if (!g_dimuat) return;   // jangan menimpa file sebelum data lama selesai dibaca
        if (!g_kotor) return;
        if (tulisKeDisk()) {
            g_kotor = false;
            if (g_kondisi == 3) g_kondisi = 1;
        } else {
            g_kondisi = 3;
            log::error("TeamGdMod: gagal menyimpan database: {}", g_pesanGalat);
        }
    }

    void flushJikaKotor() {
        if (g_kotor) flush();
    }

    bool cadangkan() {
        flush();
        std::error_code ec;
        if (!fs::exists(fileUtama(), ec)) return false;
        fs::copy_file(fileUtama(), fileManual(), fs::copy_options::overwrite_existing, ec);
        return !ec;
    }

    bool pulihkanDariCadangan() {
        std::string isi;
        std::map<std::string, std::string> hasil;
        if (!bacaFile(fileManual(), isi) || !urai(isi, hasil)) return false;
        g_data = std::move(hasil);
        g_kotor = true;
        flush();
        return true;
    }

    std::size_t jumlahKunci() { return g_data.size(); }

    std::string lokasi() { return fileUtama().string(); }

    std::string status() {
        switch (g_kondisi) {
            case 0:  return fmt::format("Baru dibuat ({} data)", g_data.size());
            case 1:  return fmt::format("Normal ({} data)", g_data.size());
            case 2:  return fmt::format("Dipulihkan dari cadangan ({} data)", g_data.size());
            default: return "Gagal menyimpan, periksa izin penyimpanan";
        }
    }
}
