# Mini Sistema de Arquivos em Memória

# Alunos
->  Lucas Emanuel Angioletti do Amaral 
->  Carlos Isidro

# Descrição
Este projeto consiste na implementação de um mini sistema de arquivos em memória, desenvolvido na linguagem C, com o objetivo de simular o funcionamento básico de um sistema de arquivos semelhante ao utilizado em sistemas Linux.
O simulador implementa diretórios em árvore, gerenciamento de arquivos, metadados (FCB), permissões de acesso, controle de usuários, alocação de blocos de dados e uma interface via terminal para interação com o sistema.
Todo o gerenciamento ocorre em memória RAM, sem utilizar o sistema de arquivos real do computador.

# Compilação
```bash
gcc -Wall -Wextra -O2 filemgmt.c -o simulador
```

ou

```bash
gcc filemgmt.c -o simulador
```

# Execução
Linux:
```bash
./simulador
```

Windows (MinGW):
```bash
simulador.exe
```

Digite `help` para listar os comandos disponíveis.

# Estruturas de Dados
## File Control Block (FCB)
Cada arquivo ou diretório possui um FCB contendo:
- ID (inode simulado)
- Nome
- Tipo
- Tamanho
- Data de criação
- Data de modificação
- Data de acesso
- Permissões (RWX)
- Proprietário
- Grupo
- Blocos utilizados

## Árvore de Diretórios
Os diretórios são representados por uma árvore utilizando ponteiros para:
- pai;
- primeiro filho;
- próximo irmão.
Essa estrutura permite navegação eficiente entre diretórios.]

## Disco Virtual
O disco é representado por:
```c
char DISCO[NUM_BLOCKS][BLOCK_SIZE];
```

e o mapa de blocos por:
```c
int blocos_livres[NUM_BLOCKS];
```

# Conceitos Implementados
## Conceito de Arquivo
Cada arquivo possui nome, inode, tipo, tamanho, proprietário, grupo, permissões e datas, armazenados no FCB.

## Operações
O simulador implementa:

- touch
- mkdir
- ls
- cd
- echo
- cat
- cp
- mv
- rm
- chmod
- su

Todas as operações ocorrem apenas na estrutura simulada em memória.

## FCB e Inode
Cada arquivo recebe um identificador único (inode simulado). O FCB concentra todos os metadados e referencia os blocos onde os dados são armazenados.

## Diretórios em Árvore
A estrutura hierárquica facilita organização, agrupamento, nomeação e navegação, semelhante aos sistemas Linux.

## Permissões RWX
O controle de acesso utiliza permissões para proprietário, grupo e outros.
Exemplo:
755

equivale a:
rwxr-xr-x

As verificações são realizadas utilizando máscaras de bits antes das operações.

## Simulação da Alocação de Blocos

Foi utilizada **alocação indexada**. O conteúdo é dividido em blocos, armazenado no disco virtual e os índices dos blocos ficam registrados no FCB.

Comparação:
- Contínua: rápida, porém sujeita à fragmentação.
- Encadeada: flexível, porém leitura mais lenta.
- Indexada: escolhida por combinar simplicidade e eficiência.

# Exemplos
Criar diretório:
mkdir documentos

Criar arquivo:
touch notas.txt

Escrever:
echo "Olá Mundo" > notas.txt

Ler:
cat notas.txt

Copiar:
cp notas.txt copia.txt

Renomear:
mv copia.txt backup.txt

Alterar permissões:
chmod 755 backup.txt

Excluir:
rm backup.txt

Todos os comandos possuem comportamento semelhante aos comandos equivalentes do Linux, porém atuam apenas sobre o sistema de arquivos simulado.

# Conclusão

O projeto demonstra os principais conceitos de sistemas de arquivos estudados na disciplina, incluindo FCB, inode, diretórios em árvore, permissões RWX, bitmasks e alocação indexada de blocos, aproximando o funcionamento interno de um sistema de arquivos real de forma didática.
