#include <iostream>
#include <map>
#include <chrono>
#include <vector>
#include <string>
#include <sstream>
#include <iomanip>     // setw, setprecision, fixed, right
#include <algorithm>   // shuffle, min, max
#include <random>      // mt19937
#include <new>         // bad_alloc
using namespace std;

struct Noeud {
    int valeur;
    Noeud* gauche;
    Noeud* droite;

    Noeud(int v) : valeur(v), gauche(nullptr), droite(nullptr) {}
};

class ABR {
private:
    Noeud* racine;

    void infixe(Noeud* n) const {
        if (n == nullptr) return;
        infixe(n->gauche);
        cout << n->valeur << " ";
        infixe(n->droite);
    }

    Noeud* supprimer(Noeud* n, int v, bool& trouve);

public:
    ABR() : racine(nullptr) {}
    void inserer(int v);
    bool supprimer(int v);
    bool rechercher(int v, int& etapes) const;
    void afficherInfixe() const { infixe(racine); cout << endl; }
};

void ABR::inserer(int v) {
    if (racine == nullptr) {
        racine = new Noeud(v);
        return;
    }
    Noeud* courant = racine;
    while (true) {
        if (v < courant->valeur) {
            if (courant->gauche == nullptr) {
                courant->gauche = new Noeud(v);
                return;
            }
            courant = courant->gauche;
        }
        else if (v > courant->valeur) {
            if (courant->droite == nullptr) {
                courant->droite = new Noeud(v);
                return;
            }
            courant = courant->droite;
        }
        else {
            return; // doublon
        }
    }
}

Noeud* ABR::supprimer(Noeud* n, int v, bool& trouve) {
    if (n == nullptr) return nullptr;

    if (v < n->valeur) {
        n->gauche = supprimer(n->gauche, v, trouve);
    }
    else if (v > n->valeur) {
        n->droite = supprimer(n->droite, v, trouve);
    }
    else {
        trouve = true;

        if (n->gauche == nullptr && n->droite == nullptr) {
            delete n;
            return nullptr;
        }
        if (n->gauche == nullptr) {
            Noeud* fils = n->droite;
            delete n;
            return fils;
        }
        if (n->droite == nullptr) {
            Noeud* fils = n->gauche;
            delete n;
            return fils;
        }
        Noeud* succ = n->droite;
        while (succ->gauche != nullptr)
            succ = succ->gauche;

        n->valeur = succ->valeur;
        n->droite = supprimer(n->droite, succ->valeur, trouve);
    }
    return n;
}

bool ABR::supprimer(int v) {
    bool trouve = false;
    racine = supprimer(racine, v, trouve);
    return trouve;
}

bool ABR::rechercher(int v, int& etapes) const {
    Noeud* courant = racine;
    etapes = 0;

    while (courant != nullptr) {
        etapes++;
        if (v == courant->valeur)
            return true;
        else if (v < courant->valeur)
            courant = courant->gauche;
        else
            courant = courant->droite;
    }
    return false;
}

int main() {
    // ===================== PARAMETRES =====================
    const vector<long long> tailles = { 1000, 10000, 100000, 1000000, 10000000 };

    // true  : les N valeurs sont inserees dans un ordre aleatoire (arbre equilibre en moyenne)
    // false : insertion croissante 1..N (arbre degenere en liste, cout O(N^2)) ;
    //         l'ABR n'est alors mesure que pour N <= LIMITE_ABR_TRIE
    const bool ORDRE_ALEATOIRE = true;
    const long long LIMITE_ABR_TRIE = 10000;

    // nombre d'operations sur lequel on moyenne la recherche et la suppression
    const long long NB_OPERATIONS = 100000;

    // ===================== STRUCTURE DES RESULTATS =====================
    struct Resultat {
        long long N;
        string statut;     // "ok", "ignore" ou "memoire insuffisante"
        double insABR, insMap;     // temps total d'insertion (s)
        double rechABR, rechMap;   // temps moyen par recherche (us)
        double suppABR, suppMap;   // temps moyen par suppression (us)
        double etapes;             // noeuds visites en moyenne par recherche (ABR)
    };
    vector<Resultat> res;
    vector<string> notes;

    // ===================== OUTILS D'AFFICHAGE =====================
    auto fmt = [](double x, int prec) -> string {
        ostringstream s;
        s << fixed << setprecision(prec) << x;
        return s.str();
    };

    auto groupe = [](long long n) -> string {   // 1000000 -> "1 000 000"
        string s = to_string(n), out;
        int c = 0;
        for (int i = (int)s.size() - 1; i >= 0; --i) {
            out.insert(out.begin(), s[i]);
            if (++c % 3 == 0 && i > 0) out.insert(out.begin(), ' ');
        }
        return out;
    };

    auto gagnant = [](double a, double b) -> string {
        if (a < b) return "ABR";
        if (b < a) return "map";
        return "egal";
    };

    auto ratio = [&](double a, double b) -> string {
        double petit = min(a, b), grand = max(a, b);
        if (petit <= 0) return "-";
        return "x" + fmt(grand / petit, 1);
    };

    auto tableau = [](const string& titre, const vector<string>& entetes,
                      const vector<vector<string> >& lignes) {
        vector<size_t> w(entetes.size());
        for (size_t c = 0; c < entetes.size(); ++c) {
            w[c] = entetes[c].size();
            for (size_t l = 0; l < lignes.size(); ++l)
                w[c] = max(w[c], lignes[l][c].size());
        }
        auto sep = [&]() {
            cout << '+';
            for (size_t c = 0; c < w.size(); ++c) cout << string(w[c] + 2, '-') << '+';
            cout << "\n";
        };
        auto ligne = [&](const vector<string>& cel) {
            cout << '|';
            for (size_t c = 0; c < w.size(); ++c)
                cout << ' ' << right << setw((int)w[c]) << cel[c] << " |";
            cout << "\n";
        };
        cout << "\n" << titre << "\n";
        sep();
        ligne(entetes);
        sep();
        for (size_t l = 0; l < lignes.size(); ++l) ligne(lignes[l]);
        sep();
    };

    // ===================== MESURES =====================
    cout << "Mesures en cours (peut etre long pour les grandes valeurs de N)..." << endl;

    for (size_t k = 0; k < tailles.size(); ++k) {
        const long long N = tailles[k];
        Resultat r = { N, "ok", 0, 0, 0, 0, 0, 0, 0 };

        if (!ORDRE_ALEATOIRE && N > LIMITE_ABR_TRIE) {
            r.statut = "ignore (insertion triee : cout O(N^2))";
            cout << "N = " << groupe(N) << " : ignore" << endl;
            res.push_back(r);
            continue;
        }

        cout << "N = " << groupe(N) << " ..." << flush;

        try {
            // permutation aleatoire de 1..N (meme graine => memes donnees pour ABR et map)
            vector<int> perm(N);
            for (long long i = 0; i < N; ++i) perm[i] = (int)(i + 1);
            mt19937 gen(42);
            shuffle(perm.begin(), perm.end(), gen);

            const long long m   = min(N, NB_OPERATIONS);          // cles interrogees
            const long long rep = max(1LL, NB_OPERATIONS / m);    // repetitions pour les petits N

            ABR arbre;
            map<int, int> mp;

            // ----- Insertion de N valeurs -----
            auto t0 = chrono::steady_clock::now();
            for (long long i = 0; i < N; ++i)
                arbre.inserer(ORDRE_ALEATOIRE ? perm[i] : (int)(i + 1));
            auto t1 = chrono::steady_clock::now();
            r.insABR = chrono::duration<double>(t1 - t0).count();

            t0 = chrono::steady_clock::now();
            for (long long i = 0; i < N; ++i)
                mp.insert({ ORDRE_ALEATOIRE ? perm[i] : (int)(i + 1),
                            ORDRE_ALEATOIRE ? perm[i] : (int)(i + 1) });
            t1 = chrono::steady_clock::now();
            r.insMap = chrono::duration<double>(t1 - t0).count();

            // ----- Recherche (cles existantes) -----
            long long trouvesABR = 0, trouvesMap = 0, totalEtapes = 0;
            int etapes = 0;

            t0 = chrono::steady_clock::now();
            for (long long j = 0; j < rep; ++j)
                for (long long i = 0; i < m; ++i) {
                    if (arbre.rechercher(perm[i], etapes)) ++trouvesABR;
                    totalEtapes += etapes;
                }
            t1 = chrono::steady_clock::now();
            r.rechABR = chrono::duration<double>(t1 - t0).count() * 1e6 / (double)(rep * m);
            r.etapes  = (double)totalEtapes / (double)(rep * m);

            t0 = chrono::steady_clock::now();
            for (long long j = 0; j < rep; ++j)
                for (long long i = 0; i < m; ++i)
                    if (mp.find(perm[i]) != mp.end()) ++trouvesMap;
            t1 = chrono::steady_clock::now();
            r.rechMap = chrono::duration<double>(t1 - t0).count() * 1e6 / (double)(rep * m);

            // ----- Suppression (m cles distinctes) -----
            long long suppA = 0, suppM = 0;

            t0 = chrono::steady_clock::now();
            for (long long i = 0; i < m; ++i)
                if (arbre.supprimer(perm[i])) ++suppA;
            t1 = chrono::steady_clock::now();
            r.suppABR = chrono::duration<double>(t1 - t0).count() * 1e6 / (double)m;

            t0 = chrono::steady_clock::now();
            for (long long i = 0; i < m; ++i)
                if (mp.erase(perm[i]) > 0) ++suppM;
            t1 = chrono::steady_clock::now();
            r.suppMap = chrono::duration<double>(t1 - t0).count() * 1e6 / (double)m;

            // ----- Verification de coherence -----
            if (trouvesABR != rep * m || trouvesMap != rep * m || suppA != m || suppM != m)
                r.statut = "ERREUR de coherence";

            cout << " termine" << endl;
        }
        catch (const bad_alloc&) {
            r.statut = "memoire insuffisante";
            cout << " memoire insuffisante" << endl;
        }

        res.push_back(r);
    }

    // ===================== CONSTRUCTION DES TABLEAUX =====================
    vector<vector<string> > tIns, tRech, tSupp;

    for (size_t i = 0; i < res.size(); ++i) {
        const Resultat& r = res[i];
        const string n = groupe(r.N);

        if (r.statut != "ok") {
            notes.push_back("N = " + n + " : " + r.statut);
            tIns.push_back({ n, "n/d", "n/d", "-", "-" });
            tRech.push_back({ n, "n/d", "n/d", "n/d", "-", "-" });
            tSupp.push_back({ n, "n/d", "n/d", "-", "-" });
            continue;
        }

        tIns.push_back({ n, fmt(r.insABR, 6), fmt(r.insMap, 6),
                         gagnant(r.insABR, r.insMap), ratio(r.insABR, r.insMap) });
        tRech.push_back({ n, fmt(r.rechABR, 3), fmt(r.rechMap, 3), fmt(r.etapes, 1),
                          gagnant(r.rechABR, r.rechMap), ratio(r.rechABR, r.rechMap) });
        tSupp.push_back({ n, fmt(r.suppABR, 3), fmt(r.suppMap, 3),
                          gagnant(r.suppABR, r.suppMap), ratio(r.suppABR, r.suppMap) });
    }

    // ===================== AFFICHAGE =====================
    cout << "\n===================== RESULTATS : ABR vs std::map =====================" << endl;
    cout << "Ordre d'insertion : " << (ORDRE_ALEATOIRE ? "aleatoire (graine 42)" : "croissant 1..N") << endl;
    cout << "Recherche et suppression : moyenne par operation sur "
         << groupe(NB_OPERATIONS) << " operations (ou N si N est plus petit)" << endl;

    tableau("Tableau 1 : Insertion de N valeurs (temps total, en secondes)",
            { "N", "ABR (s)", "map (s)", "Gagnant", "Ratio" }, tIns);

    tableau("Tableau 2 : Recherche (temps moyen par recherche, en microsecondes)",
            { "N", "ABR (us)", "map (us)", "Noeuds visites ABR", "Gagnant", "Ratio" }, tRech);

    tableau("Tableau 3 : Suppression (temps moyen par suppression, en microsecondes)",
            { "N", "ABR (us)", "map (us)", "Gagnant", "Ratio" }, tSupp);

    cout << "\nRatio = combien de fois le gagnant est plus rapide." << endl;
    for (size_t i = 0; i < notes.size(); ++i) cout << "Note - " << notes[i] << endl;

    return 0;
}
