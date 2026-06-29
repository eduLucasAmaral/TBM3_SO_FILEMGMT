#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_NAME 50
#define BLOCK_SIZE 64
#define NUM_BLOCKS 1024
#define MAX_BLOCKS_PER_FILE 8

// Definições de Cores ANSI para o Terminal
#define ANSI_RESET          "\x1b[0m"
#define ANSI_VERDE_NEGRITO  "\x1b[1;32m"
#define ANSI_AZUL_NEGRITO   "\x1b[1;34m"
#define ANSI_CIANO          "\x1b[36m"
#define ANSI_AMARELO        "\x1b[33m"
#define ANSI_VERMELHO       "\x1b[31m"
#define ANSI_ROXO           "\x1b[35m"

// Tipos de Arquivo (Mapeando explicitamente dados/programas e diretório)
typedef enum { ARQ_DADO, ARQ_PROG, DIR_TIPO } TipoArquivo;

// Estrutura para Simulação de Disco (Blocos de Dados)
char DISCO[NUM_BLOCKS][BLOCK_SIZE];
int blocos_livres[NUM_BLOCKS]; // 0 = livre, 1 = ocupado

// Estrutura do Bloco de Controle de Arquivo (FCB / Inode)
typedef struct FCB {
    int id;                           // ID único (Inodo)
    char nome[MAX_NAME];              // Nome do arquivo/diretório
    TipoArquivo tipo;                 // Tipo de arquivo
    int tamanho;                      // Tamanho em bytes
    time_t criado_em;                 // Data de criação
    time_t modificado_em;             // Última modificação
    time_t acessado_em;               // Último acesso (Leitura/cat)
    int permissoes;                   // Máscara de bits (Octal, ex: 0755)
    char dono[20];                    // Usuário dono
    char grupo[20];                   // Grupo dono
    int blocks[MAX_BLOCKS_PER_FILE];  // Alocação Indexada (índices dos blocos)
    int block_count;                  // Quantidade de blocos usados
} FCB;

// Estrutura de Nó da Árvore de Diretórios (Lista encadeada para filhos)
typedef struct No {
    FCB fcb;
    struct No* pai;                  // Ponteiro para o diretório pai
    struct No* filho;                // Primeiro filho (se for diretório)
    struct No* proximo;              // Próximo irmão na mesma pasta
} No;

// Variáveis Globais de Estado do Sistema
No* raiz = NULL;
No* diretorio_atual = NULL;
char usuario_atual[20] = "root";
char grupo_atual[20] = "root";
int proximo_inodo_id = 1;

// Inicializa o mapa de blocos do disco virtual
void init_disco() {
    for (int i = 0; i < NUM_BLOCKS; i++) {
        blocos_livres[i] = 0;
        memset(DISCO[i], 0, BLOCK_SIZE);
    }
}

// Aloca um bloco livre no disco simulado
int alocar_bloco() {
    for (int i = 0; i < NUM_BLOCKS; i++) {
        if (blocos_livres[i] == 0) {
            blocos_livres[i] = 1;
            return i;
        }
    }
    return -1; // Disco cheio
}

// Libera os blocos de dados associados a um arquivo
void liberar_blocos_arquivo(No* no) {
    for (int i = 0; i < no->fcb.block_count; i++) {
        blocos_livres[no->fcb.blocks[i]] = 0;
        memset(DISCO[no->fcb.blocks[i]], 0, BLOCK_SIZE);
    }
    no->fcb.block_count = 0;
    no->fcb.tamanho = 0;
}

// Cria um nó genérico (Arquivo ou Diretório)
No* criar_no(const char* nome, TipoArquivo tipo, No* pai) {
    No* novo = (No*)malloc(sizeof(No));
    if (!novo) return NULL;

    novo->fcb.id = proximo_inodo_id++;
    strncpy(novo->fcb.nome, nome, MAX_NAME);
    novo->fcb.tipo = tipo;
    novo->fcb.tamanho = 0;
    novo->fcb.criado_em = time(NULL);
    novo->fcb.modificado_em = time(NULL);
    novo->fcb.acessado_em = time(NULL);
    strncpy(novo->fcb.dono, usuario_atual, 20);
    strncpy(novo->fcb.grupo, grupo_atual, 20);
    novo->fcb.block_count = 0;

    // Permissões padrão: 755 para dir (rwxr-xr-x), 644 para arquivos (rw-r--r--)
    novo->fcb.permissoes = (tipo == DIR_TIPO) ? 0755 : 0644;

    novo->pai = pai;
    novo->filho = NULL;
    novo->proximo = NULL;

    return novo;
}

// Verifica permissões baseadas em bitmask: 4 = R (Ler), 2 = W (Escrever), 1 = X (Executar)
int verificar_permissao(No* no, int mascara) {
    if (strcmp(usuario_atual, "root") == 0) return 1; // Root ignora restrições

    int perm = no->fcb.permissoes;
    int perm_dono = (perm >> 6) & 7;
    int perm_grupo = (perm >> 3) & 7;
    int perm_outros = perm & 7;

    if (strcmp(usuario_atual, no->fcb.dono) == 0) {
        return (perm_dono & mascara) == mascara;
    } else if (strcmp(grupo_atual, no->fcb.grupo) == 0) {
        return (perm_grupo & mascara) == mascara;
    } else {
        return (perm_outros & mascara) == mascara;
    }
}

// Sistema Centralizado de Ajuda (Help / Manual)
void exibir_ajuda_comando(const char* comando) {
    printf("\n" ANSI_CIANO "--- MANUAL DE COMANDO ---" ANSI_RESET "\n");
    if (strcmp(comando, "mkdir") == 0) {
        printf("Uso: " ANSI_AMARELO "mkdir <nome_diretorio>" ANSI_RESET "\n");
        printf("Descrição: Cria um novo diretório vazio na pasta atual.\n");
    } else if (strcmp(comando, "touch") == 0) {
        printf("Uso: " ANSI_AMARELO "touch <nome_arquivo>" ANSI_RESET "\n");
        printf("Descrição: Cria um arquivo vazio ou atualiza timestamps se já existir.\n");
    } else if (strcmp(comando, "ls") == 0) {
        printf("Uso: " ANSI_AMARELO "ls" ANSI_RESET "\n");
        printf("Descrição: Lista arquivos/diretórios detalhando inodos, permissões e timestamps.\n");
    } else if (strcmp(comando, "cd") == 0) {
        printf("Uso: " ANSI_AMARELO "cd <nome_diretorio>  ou  cd .." ANSI_RESET "\n");
        printf("Descrição: Altera o diretório de navegação atual. '..' volta uma pasta.\n");
    } else if (strcmp(comando, "chmod") == 0) {
        printf("Uso: " ANSI_AMARELO "chmod <modo_octal> <nome>" ANSI_RESET "\n");
        printf("Descrição: Modifica as permissões de acesso usando máscaras de bits octais (ex: 755).\n");
    } else if (strcmp(comando, "echo") == 0) {
        printf("Uso: " ANSI_AMARELO "echo \"texto\" > <nome_arquivo>" ANSI_RESET "\n");
        printf("Descrição: Grava texto dividindo-o em blocos indexados físicos no disco.\n");
    } else if (strcmp(comando, "cat") == 0) {
        printf("Uso: " ANSI_AMARELO "cat <nome_arquivo>" ANSI_RESET "\n");
        printf("Descrição: Lê os blocos do arquivo e atualiza a data de acesso (`acessado_em`).\n");
    } else if (strcmp(comando, "cp") == 0) {
        printf("Uso: " ANSI_AMARELO "cp <origem> <destino>" ANSI_RESET "\n");
        printf("Descrição: Copia um arquivo criando um novo nó e duplicando seus blocos de dados no disco.\n");
    } else if (strcmp(comando, "mv") == 0) {
        printf("Uso: " ANSI_AMARELO "mv <antigo> <novo>" ANSI_RESET "\n");
        printf("Descrição: Move ou altera o nome de um arquivo/diretório existente.\n");
    } else if (strcmp(comando, "rm") == 0) {
        printf("Uso: " ANSI_AMARELO "rm <nome>" ANSI_RESET "\n");
        printf("Descrição: Exclui logicamente o nó e libera fisicamente todos os blocos alocados.\n");
    } else if (strcmp(comando, "su") == 0) {
        printf("Uso: " ANSI_AMARELO "su <usuario> <grupo>" ANSI_RESET "\n");
        printf("Descrição: Altera a identidade do usuário para testes de permissão RWX.\n");
    } else if (strcmp(comando, "clear") == 0) {
        printf("Uso: " ANSI_AMARELO "clear" ANSI_RESET "\n");
        printf("Descrição: Limpa a tela do terminal.\n");
    } else {
        printf(ANSI_VERMELHO "Nenhuma documentação detalhada para: '%s'" ANSI_RESET "\n", comando);
    }
    printf("-------------------------\n\n");
}

void exibir_ajuda_geral() {
    printf("\n" ANSI_CIANO "==================================================" ANSI_RESET "\n");
    printf(ANSI_VERDE_NEGRITO "        SIMULADOR DE SISTEMA DE ARQUIVOS   " ANSI_RESET "\n");
    printf(ANSI_CIANO "==================================================" ANSI_RESET "\n");
    printf("Execute comandos ou use " ANSI_AMARELO "<comando> /?" ANSI_RESET " para obter ajuda específica.\n\n");
    printf("  " ANSI_AMARELO "mkdir" ANSI_RESET " <nome>         | Cria diretório\n");
    printf("  " ANSI_AMARELO "touch" ANSI_RESET " <nome>         | Cria arquivo vazio\n");
    printf("  " ANSI_AMARELO "ls" ANSI_RESET "                   | Lista diretório com detalhes de tempo\n");
    printf("  " ANSI_AMARELO "cd" ANSI_RESET " <dir>             | Navega entre diretórios\n");
    printf("  " ANSI_AMARELO "chmod" ANSI_RESET " <octal> <nome> | Altera permissões de acesso\n");
    printf("  " ANSI_AMARELO "echo" ANSI_RESET " <txt> > <arq>   | Escreve dados em arquivo\n");
    printf("  " ANSI_AMARELO "cat" ANSI_RESET " <arq>            | Exibe conteúdo (atualiza acesso)\n");
    printf("  " ANSI_AMARELO "cp" ANSI_RESET " <orig> <dest>     | Copia arquivo (duplica blocos virtuais)\n");
    printf("  " ANSI_AMARELO "mv" ANSI_RESET " <ant> <novo>      | Renomeia/Move arquivo ou pasta\n");
    printf("  " ANSI_AMARELO "rm" ANSI_RESET " <nome>            | Remove arquivo ou diretório\n");
    printf("  " ANSI_AMARELO "su" ANSI_RESET " <user> <grupo>    | Altera contexto de login\n");
    printf("  " ANSI_AMARELO "clear" ANSI_RESET "                | Limpa a tela\n");
    printf("  " ANSI_AMARELO "exit" ANSI_RESET "                 | Encerra o simulador\n");
    printf(ANSI_CIANO "==================================================" ANSI_RESET "\n\n");
}

// Comando: mkdir
void cmd_mkdir(const char* nome) {
    if (diretorio_atual && !verificar_permissao(diretorio_atual, 2)) {
        printf(ANSI_VERMELHO "Erro: Permissão negada para criar diretório aqui." ANSI_RESET "\n");
        return;
    }

    No* atual = diretorio_atual->filho;
    while (atual != NULL) {
        if (strcmp(atual->fcb.nome, nome) == 0) {
            printf(ANSI_VERMELHO "Erro: O diretório ou arquivo '%s' já existe." ANSI_RESET "\n", nome);
            return;
        }
        atual = atual->proximo;
    }

    No* novo_dir = criar_no(nome, DIR_TIPO, diretorio_atual);
    novo_dir->proximo = diretorio_atual->filho;
    diretorio_atual->filho = novo_dir;
    printf("Diretório '%s' criado com sucesso.\n", nome);
}

// Comando: touch
void cmd_touch(const char* nome) {
    if (diretorio_atual && !verificar_permissao(diretorio_atual, 2)) {
        printf(ANSI_VERMELHO "Erro: Permissão negada para criar arquivo aqui." ANSI_RESET "\n");
        return;
    }

    No* atual = diretorio_atual->filho;
    while (atual != NULL) {
        if (strcmp(atual->fcb.nome, nome) == 0) {
            atual->fcb.modificado_em = time(NULL);
            atual->fcb.acessado_em = time(NULL);
            printf("Arquivo '%s' atualizou os timestamps.\n", nome);
            return;
        }
        atual = atual->proximo;
    }

    No* novo_arq = criar_no(nome, ARQ_DADO, diretorio_atual);
    novo_arq->proximo = diretorio_atual->filho;
    diretorio_atual->filho = novo_arq;
    printf("Arquivo '%s' criado com sucesso.\n", nome);
}

// Imprime permissões de forma legível (estilo ls -l)
void print_permissoes_ext(int perm) {
    printf((perm & 0400) ? "r" : "-");
    printf((perm & 0200) ? "w" : "-");
    printf((perm & 0100) ? "x" : "-");
    printf((perm & 0040) ? "r" : "-");
    printf((perm & 0020) ? "w" : "-");
    printf((perm & 0010) ? "x" : "-");
    printf((perm & 0004) ? "r" : "-");
    printf((perm & 0002) ? "w" : "-");
    printf((perm & 0001) ? "x" : "-");
}

// Comando: ls
void cmd_ls() {
    if (diretorio_atual && !verificar_permissao(diretorio_atual, 4)) {
        printf(ANSI_VERMELHO "Erro: Permissão negada para ler este diretório." ANSI_RESET "\n");
        return;
    }

    No* atual = diretorio_atual->filho;
    printf(ANSI_ROXO "Inodo\tPermissões\tDono\tGrupo\tTam(B)\tBlk\tCriado\tModif\tAcess\tNome" ANSI_RESET "\n");
    printf("-------------------------------------------------------------------------------------------------\n");
    while (atual != NULL) {
        printf("%d\t", atual->fcb.id);
        printf((atual->fcb.tipo == DIR_TIPO) ? "d" : "-");
        print_permissoes_ext(atual->fcb.permissoes);
        printf("\t%s\t%s\t%d\t%d\t", atual->fcb.dono, atual->fcb.grupo, atual->fcb.tamanho, atual->fcb.block_count);
        
        struct tm *t_cria = localtime(&atual->fcb.criado_em);
        printf("%02d-%02d %02d:%02d\t", t_cria->tm_mon+1, t_cria->tm_mday, t_cria->tm_hour, t_cria->tm_min);
        
        struct tm *t_mod = localtime(&atual->fcb.modificado_em);
        printf("%02d-%02d %02d:%02d\t", t_mod->tm_mon+1, t_mod->tm_mday, t_mod->tm_hour, t_mod->tm_min);

        struct tm *t_ace = localtime(&atual->fcb.acessado_em);
        printf("%02d-%02d %02d:%02d\t", t_ace->tm_mon+1, t_ace->tm_mday, t_ace->tm_hour, t_ace->tm_min);

        if (atual->fcb.tipo == DIR_TIPO) {
            printf(ANSI_AZUL_NEGRITO "%s" ANSI_RESET "\n", atual->fcb.nome);
        } else {
            printf("%s\n", atual->fcb.nome);
        }
        atual = atual->proximo;
    }
}

// Comando: cd
void cmd_cd(const char* nome) {
    if (strcmp(nome, "..") == 0) {
        if (diretorio_atual->pai != NULL) {
            diretorio_atual = diretorio_atual->pai;
        }
        return;
    }

    No* atual = diretorio_atual->filho;
    while (atual != NULL) {
        if (strcmp(atual->fcb.nome, nome) == 0) {
            if (atual->fcb.tipo != DIR_TIPO) {
                printf(ANSI_VERMELHO "Erro: '%s' não é um diretório." ANSI_RESET "\n", nome);
                return;
            }
            if (!verificar_permissao(atual, 1)) {
                printf(ANSI_VERMELHO "Erro: Permissão negada de acesso." ANSI_RESET "\n");
                return;
            }
            diretorio_atual = atual;
            return;
        }
        atual = atual->proximo;
    }
    printf(ANSI_VERMELHO "Erro: Diretório '%s' não encontrado." ANSI_RESET "\n", nome);
}

// Comando: chmod (CORRIGIDO)
void cmd_chmod(int modo, const char* nome) {
    No* atual = diretorio_atual->filho;
    while (atual != NULL) {
        if (strcmp(atual->fcb.nome, nome) == 0) {
            if (strcmp(usuario_atual, "root") != 0 && strcmp(usuario_atual, atual->fcb.dono) != 0) {
                printf(ANSI_VERMELHO "Erro: Apenas o dono ou o root podem alterar as permissões." ANSI_RESET "\n");
                return;
            }
            int o1 = modo / 100;
            int o2 = (modo / 10) % 10;
            int o3 = modo % 10;
            atual->fcb.permissoes = (o1 << 6) | (o2 << 3) | o3;
            atual->fcb.modificado_em = time(NULL);
            printf("Permissões de '%s' alteradas para %03o.\n", nome, atual->fcb.permissoes);
            return;
        }
        atual = atual->proximo;
    }
    printf(ANSI_VERMELHO "Erro: Arquivo ou diretório '%s' não encontrado." ANSI_RESET "\n", nome);
}

// Comando: echo "conteudo" > arquivo
void cmd_echo(const char* nome, const char* conteudo) {
    No* atual = diretorio_atual->filho;
    No* alvo = NULL;

    while (atual != NULL) {
        if (strcmp(atual->fcb.nome, nome) == 0) {
            alvo = atual;
            break;
        }
        atual = atual->proximo;
    }

    if (!alvo) {
        cmd_touch(nome);
        atual = diretorio_atual->filho;
        while (atual != NULL) {
            if (strcmp(atual->fcb.nome, nome) == 0) { alvo = atual; break; }
            atual = atual->proximo;
        }
    }

    if (!alvo || alvo->fcb.tipo == DIR_TIPO) {
        printf(ANSI_VERMELHO "Erro: Alvo inválido para escrita." ANSI_RESET "\n");
        return;
    }

    if (!verificar_permissao(alvo, 2)) {
        printf(ANSI_VERMELHO "Erro: Permissão de escrita negada." ANSI_RESET "\n");
        return;
    }

    liberar_blocos_arquivo(alvo);

    int tam_conteudo = strlen(conteudo);
    int bytes_escritos = 0;

    while (bytes_escritos < tam_conteudo && alvo->fcb.block_count < MAX_BLOCKS_PER_FILE) {
        int bloco_idx = alocar_bloco();
        if (bloco_idx == -1) {
            printf(ANSI_VERMELHO "Erro: Disco Virtual cheio!" ANSI_RESET "\n");
            break;
        }

        alvo->fcb.blocks[alvo->fcb.block_count] = bloco_idx;
        int pedaco = (tam_conteudo - bytes_escritos > BLOCK_SIZE) ? BLOCK_SIZE : (tam_conteudo - bytes_escritos);
        
        strncpy(DISCO[bloco_idx], conteudo + bytes_escritos, pedaco);
        bytes_escritos += pedaco;
        alvo->fcb.block_count++;
    }

    alvo->fcb.tamanho = bytes_escritos;
    alvo->fcb.modificado_em = time(NULL);
    alvo->fcb.acessado_em = time(NULL);
    printf("Escritos %d bytes em %d bloco(s) no arquivo '%s'.\n", bytes_escritos, alvo->fcb.block_count, nome);
}

// Comando: cat
void cmd_cat(const char* nome) {
    No* atual = diretorio_atual->filho;
    while (atual != NULL) {
        if (strcmp(atual->fcb.nome, nome) == 0) {
            if (atual->fcb.tipo == DIR_TIPO) {
                printf(ANSI_VERMELHO "Erro: '%s' é um diretório." ANSI_RESET "\n", nome);
                return;
            }
            if (!verificar_permissao(atual, 4)) {
                printf(ANSI_VERMELHO "Erro: Permissão de leitura negada." ANSI_RESET "\n");
                return;
            }

            atual->fcb.acessado_em = time(NULL);

            printf(ANSI_CIANO "--- Conteúdo de %s ---" ANSI_RESET "\n", nome);
            int total_bytes = atual->fcb.tamanho;
            int bytes_lidos = 0;

            for (int i = 0; i < atual->fcb.block_count; i++) {
                int bloco_idx = atual->fcb.blocks[i];
                int pedaco = (total_bytes - bytes_lidos > BLOCK_SIZE) ? BLOCK_SIZE : (total_bytes - bytes_lidos);
                for (int j = 0; j < pedaco; j++) {
                    putchar(DISCO[bloco_idx][j]);
                }
                bytes_lidos += pedaco;
            }
            printf("\n" ANSI_CIANO "---------------------" ANSI_RESET "\n");
            return;
        }
        atual = atual->proximo;
    }
    printf(ANSI_VERMELHO "Erro: Arquivo '%s' não encontrado." ANSI_RESET "\n", nome);
}

// Comando: cp
void cmd_cp(const char* origen, const char* destino) {
    No* atual = diretorio_atual->filho;
    No* no_origem = NULL;
    No* no_destino_check = NULL;

    while (atual != NULL) {
        if (strcmp(atual->fcb.nome, origen) == 0) no_origem = atual;
        if (strcmp(atual->fcb.nome, destino) == 0) no_destino_check = atual;
        atual = atual->proximo;
    }

    if (!no_origem) {
        printf(ANSI_VERMELHO "Erro: Arquivo de origem '%s' não encontrado." ANSI_RESET "\n", origen);
        return;
    }
    if (no_origem->fcb.tipo == DIR_TIPO) {
        printf(ANSI_VERMELHO "Erro: '%s' é um diretório (cópia não suportada)." ANSI_RESET "\n", origen);
        return;
    }
    if (!verificar_permissao(no_origem, 4)) {
        printf(ANSI_VERMELHO "Erro: Permissão de leitura negada na origem." ANSI_RESET "\n");
        return;
    }
    if (no_destino_check) {
        printf(ANSI_VERMELHO "Erro: O alvo de destino '%s' já existe." ANSI_RESET "\n", destino);
        return;
    }
    if (diretorio_atual && !verificar_permissao(diretorio_atual, 2)) {
        printf(ANSI_VERMELHO "Erro: Sem permissão de escrita para criar o arquivo destino." ANSI_RESET "\n");
        return;
    }

    No* novo_dest = criar_no(destino, no_origem->fcb.tipo, diretorio_atual);
    if (!novo_dest) return;

    int falha_disco = 0;
    for (int i = 0; i < no_origem->fcb.block_count; i++) {
        int bloco_novo = alocar_bloco();
        if (bloco_novo == -1) {
            printf(ANSI_VERMELHO "Erro: Disco cheio ao copiar blocos físicos!" ANSI_RESET "\n");
            falha_disco = 1;
            break;
        }
        memcpy(DISCO[bloco_novo], DISCO[no_origem->fcb.blocks[i]], BLOCK_SIZE);
        novo_dest->fcb.blocks[novo_dest->fcb.block_count] = bloco_novo;
        novo_dest->fcb.block_count++;
    }

    if (falha_disco) {
        liberar_blocos_arquivo(novo_dest);
        free(novo_dest);
        return;
    }

    novo_dest->fcb.tamanho = no_origem->fcb.tamanho;
    novo_dest->proximo = diretorio_atual->filho;
    diretorio_atual->filho = novo_dest;
    
    no_origem->fcb.acessado_em = time(NULL);
    printf("Arquivo '%s' copiado com sucesso para '%s'.\n", origen, destino);
}

// Comando: mv
void cmd_mv(const char* antigo, const char* novo) {
    if (diretorio_atual && !verificar_permissao(diretorio_atual, 2)) {
        printf(ANSI_VERMELHO "Erro: Sem permissão de modificação neste diretório." ANSI_RESET "\n");
        return;
    }

    No* atual = diretorio_atual->filho;
    No* no_alvo = NULL;
    No* no_existente_check = NULL;

    while (atual != NULL) {
        if (strcmp(atual->fcb.nome, antigo) == 0) no_alvo = atual;
        if (strcmp(atual->fcb.nome, novo) == 0) no_existente_check = atual;
        atual = atual->proximo;
    }

    if (!no_alvo) {
        printf(ANSI_VERMELHO "Erro: '%s' não encontrado." ANSI_RESET "\n", antigo);
        return;
    }
    if (no_existente_check) {
        printf(ANSI_VERMELHO "Erro: Já existe um item com o nome '%s'." ANSI_RESET "\n", novo);
        return;
    }

    strncpy(no_alvo->fcb.nome, novo, MAX_NAME);
    no_alvo->fcb.modificado_em = time(NULL);

    printf("'%s' renomeado com sucesso para '%s'.\n", antigo, novo);
}

// Comando: rm
void cmd_rm(const char* nome) {
    if (!verificar_permissao(diretorio_atual, 2)) {
        printf(ANSI_VERMELHO "Erro: Sem permissão de escrita no diretório para exclusão." ANSI_RESET "\n");
        return;
    }

    No* atual = diretorio_atual->filho;
    No* anterior = NULL;

    while (atual != NULL) {
        if (strcmp(atual->fcb.nome, nome) == 0) {
            if (atual->fcb.tipo == DIR_TIPO && atual->filho != NULL) {
                printf(ANSI_VERMELHO "Erro: O diretório não está vazio." ANSI_RESET "\n");
                return;
            }
            if (!verificar_permissao(atual, 2)) {
                printf(ANSI_VERMELHO "Erro: Sem permissão para remover '%s'." ANSI_RESET "\n", nome);
                return;
            }

            if (anterior == NULL) {
                diretorio_atual->filho = atual->proximo;
            } else {
                anterior->proximo = atual->proximo;
            }

            if (atual->fcb.tipo != DIR_TIPO) {
                liberar_blocos_arquivo(atual);
            }

            free(atual);
            printf("'%s' removido com sucesso.\n", nome); 
            return;
        }
        anterior = atual;
        atual = atual->proximo;
    }
    printf(ANSI_VERMELHO "Erro: '%s' não encontrado." ANSI_RESET "\n", nome);
}

// Monta o Prompt Customizado e Colorido
void mostrar_prompt() {
    char caminho[512] = "";
    No* curr = diretorio_atual;
    if (curr == raiz) {
        strcpy(caminho, "/");
    } else {
        char temp[512] = "";
        while (curr != raiz) {
            char passo[60];
            sprintf(passo, "/%s", curr->fcb.nome);
            strcat(passo, temp);
            strcpy(temp, passo);
            curr = curr->pai;
        }
        strcpy(caminho, temp);
    }
    
    if (strcmp(usuario_atual, "root") == 0) {
        printf("[" ANSI_VERMELHO "%s" ANSI_RESET "@lunix " ANSI_AZUL_NEGRITO "%s" ANSI_RESET "]$ ", usuario_atual, caminho);
    } else {
        printf("[" ANSI_VERDE_NEGRITO "%s" ANSI_RESET "@lunix " ANSI_AZUL_NEGRITO "%s" ANSI_RESET "]$ ", usuario_atual, caminho);
    }
}

int main() {
    init_disco();
    
    raiz = criar_no("/", DIR_TIPO, NULL);
    diretorio_atual = raiz;

    printf("\033[H\033[J");

    printf(ANSI_CIANO "==================================================" ANSI_RESET "\n");
    printf(ANSI_VERDE_NEGRITO "   SIMULADOR DE SISTEMA DE ARQUIVOS EM MEMÓRIA     " ANSI_RESET "\n");
    printf(ANSI_VERDE_NEGRITO "   Sistemas Operacionais - Ciência da Computação   " ANSI_RESET "\n");
    printf(ANSI_CIANO "==================================================" ANSI_RESET "\n");
    printf("Digite '" ANSI_AMARELO "help" ANSI_RESET "' para ver a lista de comandos.\n");
    printf("Você também pode usar '" ANSI_AMARELO "<comando> /?" ANSI_RESET "' para ajuda específica.\n\n");

    char linha_comando[512];
    while (1) {
        mostrar_prompt();
        if (!fgets(linha_comando, sizeof(linha_comando), stdin)) break;

        linha_comando[strcspn(linha_comando, "\n")] = 0;

        char cmd[50] = "";
        char arg1[256] = "";
        char arg2[256] = "";
        char extra[50] = "";

        if (strncmp(linha_comando, "echo ", 5) == 0) {
            char* seta = strchr(linha_comando, '>');
            if (seta != NULL) {
                if (strstr(linha_comando, "/?") != NULL) {
                    exibir_ajuda_comando("echo");
                    continue;
                }
                *seta = '\0';
                char* ptr_conteudo = linha_comando + 5;
                char* ptr_arquivo = seta + 1;

                while(*ptr_conteudo == ' ') ptr_conteudo++;
                while(*ptr_arquivo == ' ') ptr_arquivo++;
                
                if(ptr_conteudo[0] == '"' || ptr_conteudo[0] == '\'') ptr_conteudo++;
                int len = strlen(ptr_conteudo);
                if(len > 0 && (ptr_conteudo[len-1] == '"' || ptr_conteudo[len-1] == '\'')) ptr_conteudo[len-1] = '\0';

                cmd_echo(ptr_arquivo, ptr_conteudo);
                continue;
            }
        }

        int tokens = sscanf(linha_comando, "%s %s %s %s", cmd, arg1, arg2, extra);
        if (tokens <= 0) continue;

        if (strcmp(arg1, "/?") == 0) {
            exibir_ajuda_comando(cmd);
            continue;
        }

        if (strcmp(cmd, "exit") == 0) {
            break;
        } else if (strcmp(cmd, "help") == 0) {
            exibir_ajuda_geral();
        } else if (strcmp(cmd, "clear") == 0) {
            printf("\033[H\033[J");
        } else if (strcmp(cmd, "mkdir") == 0 && tokens >= 2) {
            cmd_mkdir(arg1);
        } else if (strcmp(cmd, "touch") == 0 && tokens >= 2) {
            cmd_touch(arg1);
        } else if (strcmp(cmd, "ls") == 0) {
            cmd_ls();
        } else if (strcmp(cmd, "cd") == 0 && tokens >= 2) {
            cmd_cd(arg1);
        } else if (strcmp(cmd, "chmod") == 0 && tokens >= 3) {
            cmd_chmod(atoi(arg1), arg2);
        } else if (strcmp(cmd, "cat") == 0 && tokens >= 2) {
            cmd_cat(arg1);
        } else if (strcmp(cmd, "cp") == 0 && tokens >= 3) {
            cmd_cp(arg1, arg2);
        } else if (strcmp(cmd, "mv") == 0 && tokens >= 3) {
            cmd_mv(arg1, arg2);
        } else if (strcmp(cmd, "rm") == 0 && tokens >= 2) {
            cmd_rm(arg1);
        } else if (strcmp(cmd, "su") == 0 && tokens >= 3) {
            strncpy(usuario_atual, arg1, 20);
            strncpy(grupo_atual, arg2, 20);
            printf("Troca de contexto efetuada: %s (%s)\n", usuario_atual, grupo_atual);
        } else {
            printf(ANSI_VERMELHO "Comando desconhecido ou parâmetros insuficientes. Digite 'help' para ajuda." ANSI_RESET "\n");
        }
    }
    return 0;
}