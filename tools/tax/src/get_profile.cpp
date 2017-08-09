#include <iostream>
#include <chrono>
#include <set>
#include <map>
#include <random>
#include "config_get_profile.h"
#include "file_list_loader.h"
#include "seq_transform.h"
#include "io.h"

typedef uint64_t hash_t;

#include "hash.h"
#include "fasta.h"

#include "omp_adapter.h"

using namespace std;
using namespace std::chrono;

struct MinHash
{
    struct Best
    {
        uint64_t hash = UINT64_MAX;
        hash_t kmer = 0;
        Best() = default;
        Best(uint64_t hash, hash_t kmer) : hash(hash), kmer(kmer){}
    };

    vector<Best> best; //, storage;
    vector<uint64_t> storage_hash; // memory fetch optimization
    vector<hash_t> storage_kmer;

    vector<uint64_t> xors;

    MinHash(size_t count) : best(count), xors(count)
    {
        std::mt19937 rng;
        rng.seed(0);
        std::uniform_int_distribution<std::mt19937::result_type> dist(0, UINT32_MAX);
        for (int i = 0; i < count; i++)
            xors[i] = (uint64_t(dist(rng)) << 32) | dist(rng);

        storage_hash.reserve(10000000); // todo: think. filesize ?
        storage_kmer.reserve(10000000); // todo: think. filesize ?
    }

    void add(uint64_t hash, hash_t kmer)
    {
        storage_hash.push_back(hash);
        storage_kmer.push_back(kmer);
    }

    void finish()
    {
        #pragma omp parallel for
        for (int ib = 0; ib < best.size(); ib ++)
        {
            auto _xor = xors[ib];

            const int BUCKETS = 4;
            vector<Best> best_chosen;
            for (int i = 0; i < BUCKETS; i++)
                best_chosen.push_back(best[ib]);

            auto storage_limit = (storage_hash.size() / BUCKETS) * BUCKETS;
            for (size_t to_check_i = 0; to_check_i < storage_limit; to_check_i += BUCKETS)
            {
                union
                {
                    __uint128_t h128;
                    struct
                    {
                        hash_t h0, h1;
                    } h64;
                } h01, h23;

                h01.h128 = *(__uint128_t*)&storage_hash[to_check_i + 0];
                h23.h128 = *(__uint128_t*)&storage_hash[to_check_i + 2];
//                h.h128 ^= _xor128; - slower
                hash_t h0 = h01.h64.h0 ^ _xor;
                hash_t h1 = h01.h64.h1 ^ _xor;
                hash_t h2 = h23.h64.h0 ^ _xor;
                hash_t h3 = h23.h64.h1 ^ _xor;

                if (h0 < best_chosen[0].hash)
                    best_chosen[0] = Best(h0, storage_kmer[to_check_i + 0]);
                if (h1 < best_chosen[1].hash)
                    best_chosen[1] = Best(h1, storage_kmer[to_check_i + 1]);
                if (h2 < best_chosen[2].hash)
                    best_chosen[2] = Best(h2, storage_kmer[to_check_i + 2]);
                if (h3 < best_chosen[3].hash)
                    best_chosen[3] = Best(h3, storage_kmer[to_check_i + 3]);
            }


            for (size_t to_check_i = storage_limit; to_check_i < storage_hash.size(); to_check_i ++)
            {
                hash_t h0 = storage_hash[to_check_i + 0] ^ _xor;

                if (h0 < best_chosen[0].hash)
                    best_chosen[0] = Best(h0, storage_kmer[to_check_i + 0]);
            }

            for (int i = 0; i < BUCKETS; i++)
                if (best_chosen[i].hash < best[ib].hash)
                    best[ib] = best_chosen[i];
        }

        storage_hash.clear();
        storage_kmer.clear();
    }
};

uint64_t fnv1_hash (void *key, int n_bytes)
{
    unsigned char *p = (unsigned char *)key;
    uint64_t h = 14695981039346656037UL;

    for (int i = 0; i < n_bytes; i++)
        h = (h * 1099511628211) ^ p[i];

    return h;
}

hash_t hash_of(hash_t hash)
{
	return fnv1_hash(&hash, sizeof(hash));
}

void update_min_hash(MinHash &min_hash, const string &seq, int kmer_len)
{
    cerr << '.';

    Hash<hash_t>::for_all_hashes_do(seq, kmer_len, [&](hash_t hash)
    {
        hash = seq_transform<hash_t>::min_hash_variant(hash, kmer_len);
        min_hash.add(hash_of(hash), hash);
        return true;
    });
}

void save(const string &filename, const MinHash &min_hash)
{
    cout << "saving to " << filename << endl;
    std::ofstream f(filename, std::ios::out | std::ios::binary);
    IO::write(f, min_hash.best.size());
    for (auto &b : min_hash.best)
        IO::write(f, b.kmer);
}

string nodir(const string &filename)
{
    auto pos = filename.find_last_of('/');
    if (pos == string::npos)
        return filename;

    return filename.substr(pos + 1);
}

string save_file(const string &filename)
{
    return string("./") + nodir(filename) + ".profile";
}

bool file_exists(const string &filename)
{
    ifstream f(filename);
    return f.good();
}

void get_profile(const string &filename, int kmer_len, int min_hash_count)
{
    MinHash min_hash(min_hash_count);

    cout << "loading " << filename << endl;

    Fasta fasta(filename);
    string seq;

    while (fasta.get_next_sequence(seq))
        update_min_hash(min_hash, seq, kmer_len);

    min_hash.finish();

    cout << endl;
    save(save_file(filename), min_hash);
}

void get_profile(const Config &config)
{
	FileListLoader file_list(config.file_list);

//   	#pragma omp parallel for
    for (int file_number = 0; file_number < int(file_list.files.size()); file_number ++)
    {
        auto &file = file_list.files[file_number];
        get_profile(file.filename, config.kmer_len, config.min_hash_count);
    }
}

int main(int argc, char const *argv[])
{
	Config config(argc, argv);

	auto before = high_resolution_clock::now();

    try
    {
        get_profile(config);
    }
    catch (std::exception &ex)
    {
        cerr << ex.what() << endl;
    }

	cerr << "total time (sec) " << std::chrono::duration_cast<std::chrono::seconds>( high_resolution_clock::now() - before ).count() << endl;
}
