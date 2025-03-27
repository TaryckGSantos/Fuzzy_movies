#include <iostream>
#include <fstream>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <sstream>
#include <iomanip>
#include <tuple>
#include <algorithm>
using namespace std;

// Função para dividir uma linha do CSV em colunas, considerando aspas para campos com vírgula
vector<string> split_csv_line(const string& linha) {
    vector<string> colunas;
    string campo;
    bool dentro_aspas = false;

    for (size_t i = 0; i < linha.size(); ++i) {
        char c = linha[i];

        if (c == '"') {
            dentro_aspas = !dentro_aspas;
        } else if (c == ',' && !dentro_aspas) {
            colunas.push_back(campo);
            campo.clear();
        } else {
            campo += c;
        }
    }

    colunas.push_back(campo);

    for (auto& col : colunas) {
        if (!col.empty() && col.front() == '"' && col.back() == '"') {
            col = col.substr(1, col.size() - 2);
        }
    }

    return colunas;
}

// Função auxiliar para dividir uma string com base em espaços (usada para keywords e gêneros)
vector<string> split_por_espaco(const string& texto) {
    vector<string> partes;
    stringstream ss(texto);
    string temp;
    while (ss >> temp) {
        partes.push_back(temp);
    }
    return partes;
}


int main() {

    // Palavras comuns a serem ignoradas na análise das palavras-chave (stop words)
    unordered_set<string> stop_words = {
        "the", "a", "an", "of", "and", "on", "in", "at", "for", "with",
        "to", "by", "from", "about", "as", "into", "like", "over", "after",
        "under", "up", "down", "off", "out", "near", "without", "through",
        "based", "novel", "story", "film", "movie", "credits", "aftercreditsstinger",
        "duringcreditsstinger", "3d"
    };

    // Abre o arquivo original contendo os dados dos filmes
    ifstream arquivo("filmes.csv");
    if (!arquivo.is_open()) {
        cerr << "Erro ao abrir o arquivo." << endl;
        return 1;
    }

    // Cria um novo CSV para salvar os dados originais com uma nova coluna "score"
    ofstream saida_csv("filmes_com_score.csv");
    if (!saida_csv.is_open()) {
        cerr << "Erro ao criar filmes_com_score.csv." << endl;
        return 1;
    }

    // Mapa de preferências de gêneros com valores atribuídos de acordo com gosto pessoal
    unordered_map<string, int> genero_preferencia = {
        {"Drama", 1}, {"Comedy", 10}, {"Thriller", 6}, {"Action", 10}, {"Romance", 7},
        {"Adventure", 8}, {"Crime", 6}, {"Science_Fiction", 8}, {"Horror", 5},
        {"Family", 5}, {"Fantasy", 7}, {"Mistery", 7}, {"Animation", 8}, {"History", 4},
        {"Music", 5}, {"War", 8}, {"Documentary", 4}, {"Western", 1}, {"Foreign", 5},
        {"TV_Movie", 0}
    };

    // Índices dos filmes favoritos (top 10), e seus respectivos pesos decrescentes
    vector<int> favoritos = {95, 191, 117, 1356, 113, 566, 899, 4722, 1079, 1193};
    vector<float> pesos = {1.0, 0.95, 0.9, 0.85, 0.8, 0.75, 0.7, 0.65, 0.6, 0.55};

    // Mapeamento das palavras-chave dos filmes favoritos com suas relevâncias fuzzy
    unordered_map<string, float> palavra_relevancia;
    string linha;
    getline(arquivo, linha); // pula o cabeçalho
    saida_csv << linha << ",score\n"; // cabeçalho novo

    // Lê todas as linhas do arquivo CSV e armazena cada linha como uma string no vetor todas_linhas
    vector<string> todas_linhas;
    while (getline(arquivo, linha)) {
        todas_linhas.push_back(linha);
    }

    // Percorre todos os filmes e acumula a pontuação das palavras-chave com base nos favoritos
    for (const string& linha : todas_linhas) {
        vector<string> colunas = split_csv_line(linha);
        if (colunas.size() != 5) continue;

        int index;
        try { index = stoi(colunas[1]); } catch (...) { continue; }

        for (size_t j = 0; j < favoritos.size(); ++j) {
            if (index == favoritos[j]) {
                vector<string> palavras = split_por_espaco(colunas[3]); // keywords
                for (const string& p : palavras) {
                    if (stop_words.count(p) == 0) {
                        palavra_relevancia[p] += pesos[j];
                    }
                }
            }
        }
    }

    // Lista de filmes recomendados com seus scores (excluindo os favoritos)
    vector<tuple<float, string, string>> recomendados;

    // Loop para calcular o score de cada filme e salvar no novo CSV
    for (const string& linha : todas_linhas) {
        vector<string> colunas = split_csv_line(linha);
        if (colunas.size() != 5) continue;

        string genres = colunas[2];
        string keywords = colunas[3];

        vector<string> lista_generos = split_por_espaco(genres);
        float soma_notas = 0;
        int qtd_generos_validos = 0;
        for (const string& genero : lista_generos) {
            if (genero_preferencia.count(genero)) {
                soma_notas += genero_preferencia[genero];
                qtd_generos_validos++;
            }
        }
        if (qtd_generos_validos == 0) continue;
        float score_genero = soma_notas / qtd_generos_validos;

        vector<string> lista_keywords = split_por_espaco(keywords);
        float soma_relevancia = 0;
        int palavras_relevantes = 0;
        for (const string& palavra : lista_keywords) {
            if (stop_words.count(palavra) == 0 && palavra_relevancia.count(palavra)) {
                soma_relevancia += palavra_relevancia[palavra];
                palavras_relevantes++;
            }
        }
        float proporcao = lista_keywords.empty() ? 0.0 : (float)palavras_relevantes / lista_keywords.size();
        float score_keywords = soma_relevancia * proporcao;
        float score_final = (score_genero + score_keywords) / 2;

        saida_csv << linha << "," << fixed << setprecision(2) << score_final << "\n";

        int index;
        try { index = stoi(colunas[1]); } catch (...) { continue; }
        if (find(favoritos.begin(), favoritos.end(), index) == favoritos.end()) {
            recomendados.emplace_back(score_final, colunas[1], colunas[4]);
        }
    }

    // Fecha os arquivos
    saida_csv.close();
    arquivo.close();

    // Exibe os detalhes dos filmes favoritos
    cout << "\n\n10 filmes favoritos:\n";
    for (int i = 0; i < favoritos.size(); ++i) {
        for (const string& linha : todas_linhas) {
            vector<string> colunas = split_csv_line(linha);
            if (colunas.size() != 5) continue;
            int idx;
            try { idx = stoi(colunas[1]); } catch (...) { continue; }

            if (idx == favoritos[i]) {
                string index = colunas[1];
                string genres = colunas[2];
                string keywords = colunas[3];
                string title = colunas[4];

                vector<string> lista_generos = split_por_espaco(genres);

                vector<string> lista_keywords = split_por_espaco(keywords);
                float soma_relevancia = 0;
                int palavras_relevantes = 0;
                for (const string& palavra : lista_keywords) {
                    if (stop_words.count(palavra) == 0 && palavra_relevancia.count(palavra)) {
                        soma_relevancia += palavra_relevancia[palavra];
                        palavras_relevantes++;
                    }
                }
                float proporcao = lista_keywords.empty() ? 0.0 : (float)palavras_relevantes / lista_keywords.size();
                
                cout << i+1 << "º - " << title << " (Index: " << index << ")\n";
                cout << "Gêneros: " << genres << "\n";
                cout << "Palavras-chave: " << keywords << "\n";
                cout << "-----------------------------\n";
            }
        }
    }

    // Mostra as preferências de gêneros
    cout << "\nPreferências de gêneros (peso):\n";
    for (const auto& par : genero_preferencia) {
        cout << "- " << par.first << ": " << par.second << endl;
    }

    // Exibe as palavras-chave dos favoritos com a pontuação fuzzy acumulada
    cout << endl << "Palavras-chave dos filmes favoritos | pontuação:\n";
    vector<pair<string, float>> palavras_sorted(palavra_relevancia.begin(), palavra_relevancia.end());
    sort(palavras_sorted.begin(), palavras_sorted.end(), [](auto& a, auto& b) {
        return a.second > b.second;
    });
    for (const auto& p : palavras_sorted) {
        cout << "- " << p.first << ": " << p.second << endl;
    }

    sort(recomendados.rbegin(), recomendados.rend());

    // Ordena e exibe os 10 filmes recomendados com maiores scores
    cout << endl << "Top 10 filmes recomendados:\n";
    for (int i = 0; i < 10 && i < recomendados.size(); ++i) {
        float score = get<0>(recomendados[i]);
        string idx = get<1>(recomendados[i]);
        string title = get<2>(recomendados[i]);
        cout << i + 1 << "º - " << title << " (Index: " << idx << ")\n";
        for (const string& linha : todas_linhas) {
            vector<string> col = split_csv_line(linha);
            if (col.size() != 5) continue;
            if (col[1] == idx) {
                cout << "Gêneros: " << col[2] << endl;
                cout << "Palavras-chave: " << col[3] << endl;
                cout << "Score final: " << fixed << setprecision(2) << score << endl;
                cout << "-----------------------------" << endl;
                break;
            }
        }
    }

    return 0;
}
