# OpenPocket Bass (M-VAVE BlackBox)

Uma versão do OpenPocket pensada para baixo. A cadeia dos presets de fábrica é:

    PITCH (BASS OCTAVE)  ->  EQ (BASS PEQ)  ->  AMP (NAM)  ->  CAB (USER IR + BLEND)  ->  MOD (BASS CHORUS)

Os outros blocos continuam no firmware, só que desligados nos presets de baixo. O compressor, por exemplo, fica à mão no preset SLAP.

## O que mudou em relação ao OpenPocket

| Onde | O quê |
| --- | --- |
| EQ, tipo **BASS PEQ** | LOW (shelf em 80 Hz, ±12 dB) · MID FREQ (**40 Hz a 4 kHz**) · MID GAIN (±12 dB) · MID Q (0.4 a 6.0) · HIGH (shelf em 3,5 kHz, ±12 dB). O PARAMETRIC original só descia até 150 Hz. |
| CAB, tipo **USER IR** | Ganhou **BLEND** (0 = só amp+IR, 100 = só a outra fonte) e **BLEND SRC**, que escolhe a fonte: **DI** é o baixo limpo pego na entrada do amp, então o BLEND doseia o amp e o IR juntos. **AMP** é a saída do amp sem o IR, então o BLEND doseia só a caixa. **DI INV** e **AMP INV** são as mesmas fontes com a polaridade invertida. |
| PITCH, tipo **BASS OCTAVE** | Oitavador analógico de uma nota só (divisor flip-flop): DRY · SUB −1 · UP +1 · TONE (60 a 600 Hz, filtro do sub) · TRACK (100 a 500 Hz; mais baixo deixa o rastreio mais firme nas cordas graves). O disparo tem histerese pra não "gaguejar" no fim da nota, e o filtro da oitava acima acompanha a nota tocada. Fica **antes do EQ**, onde rastreia o baixo limpo. |
| MOD, tipo **BASS CHORUS** | RATE · DEPTH · MIX · **X-OVER** (50 a 500 Hz). Abaixo do X-OVER o grave passa seco, no estilo dos chorus de baixo. |
| Afinador | Agora lê de **27,6 Hz a 551 Hz**: Si grave (30,9 Hz) do baixo de 5 cordas, Mi grave (41,2 Hz) e harmônicos na 12ª casa. O original começava em 61 Hz. |
| Presets | 01 DI LIMPO · 02 AMP 1 · 03 AMP 1 BLEND · 04 AMP 2 · 05 AMP 2 BLEND · 06 GROWL · 07 SLAP · 08 CHORUS · 09 AMP 3 · 10 AMP 4 · 11 OITAVA · 12–52 USER (EQ → U1 → IR1, tudo flat) |
| Tela de boot | Mostra `v42 BASS`. |

## Como usar o BLEND

1. Carregue uma captura NAM **A2** de amp de baixo no slot U1 e um IR no slot IR1, pelo editor web (painéis "NAM models" e "IRs").
2. Abra o preset **03 AMP 1 BLEND**. O IR tem LOW CUT em 80 Hz, então o grave vem do sinal limpo e o amp entra por cima.
3. Gire o **BLEND**. Se o grave **diminuir** quando o BLEND chega perto do meio, troque **BLEND SRC** para DI INV (ou AMP INV). Algumas capturas invertem a polaridade do grave. Nos 12 amps de fábrica, por exemplo, 8 pedem INVERT (veja `tests/engine/bass_test.c`, parte 3).
4. Acerte o volume final no **PRESET LVL** (página GT, 3º ajuste).

O limpo e o amp chegam alinhados no tempo também no modo de dois núcleos: o firmware guarda o limpo de cada bloco de áudio junto com a saída do NAM (`a_cln` em `firmware/px_audio.h`). O teste confere isso bit a bit.

## Compilar

Igual ao OpenPocket (`docs/BUILDING.md`). `make package` gera `docs/firmware/BlackBox_942.fwsc` já com os presets de baixo. `OP_BASS=0` volta para os presets de guitarra.

## Testes

`tests/engine/bass_test.c` (roda em `make test`):

- BLEND pelo pipeline de dois núcleos, comparado com o motor simples: idêntico, um bloco depois;
- BLEND 0 não muda nada do som original. O motor foi conferido bit a bit contra o OpenPocket v42 em 852 rodadas com tipos e ajustes antigos;
- BLEND 100 = só o limpo; INVERT = o limpo com a polaridade invertida;
- BASS CHORUS deixa um Mi de 41 Hz intacto e faz o chorus em 1 kHz;
- BASS OCTAVE: em notas dedilhadas de B0 a C3 (com o 2º harmônico mais forte que a fundamental), o SUB −1 fica de 32 a 47 dB acima de qualquer outra frequência. O UP +1 fica de 8 a 15 dB acima: uma oitava acima analógica nunca sai pura;
- BASS PEQ: o MID corta e reforça a partir de 40 Hz;
- afinador: B0, E1, A1, D2, G2 e C3 com erro de até 0,5 cent; harmônicos da 12ª casa com até 2,1 cents.

## Antes de instalar: backup da BlackBox

Instalar o OpenPocket apaga os amps, IRs e presets guardados na pedaleira. O firmware usa a mesma parte da memória flash (`docs/HARDWARE-HANDBOOK.md`, seção 3). Por isso, com o **firmware oficial** ainda instalado:

1. abra `backup.html` (no Chrome ou no Edge, com a BlackBox no USB) e clique em **Conectar** e depois em **Fazer backup**;
2. guarde o `.zip` que ele baixa.

A página **só lê** a pedaleira: usa o mesmo comando de leitura (`0x23`) que o verificador dos sons de fábrica da V21 (`docs/pxfactory.js`) e nunca grava nada. O banco de amps e IRs é lido duas vezes, e as duas leituras têm que dar iguais. O zip traz:

- `ampcab.bin`: os 20 amps e os 20 IRs byte a byte;
- `presets.bin`: os 80 presets;
- cada slot também num arquivo separado;
- `info.json`, com os nomes de tudo e o SHA-256 de cada arquivo.

Teste: `tests/test_backup.mjs`, contra uma BlackBox simulada.

Pra voltar ao firmware oficial, use o passo 3 da página de instalação com o `.fwsc` da M-Vave. Os sons não voltam sozinhos: o firmware oficial só traz o programa. A restauração do teu banco a partir desse backup ainda vai ser escrita. Até lá, guarde também os arquivos originais das tuas capturas e IRs.

## Seus sons da M-Vave ficam guardados (BlackBox)

O OpenPocket usa a parte da memória onde a M-Vave guarda os 20 amps, os 20 IRs e os 80 presets. Por isso:

- **Antes de instalar** por cima do firmware oficial, a página de instalação lê tudo duas vezes (as leituras têm que
  bater), baixa o backup (.zip) e guarda uma cópia no navegador. Se o backup falhar, nada é instalado.
- **Na volta pro firmware oficial V20** (passo 3), a página regrava sozinha os amps, IRs e presets do backup, só os
  slots que mudaram, e confere cada um lendo de volta. O botão "Regravar meus sons" faz o mesmo a qualquer hora; sem
  o backup no navegador, escolha o .zip.
- A regravação usa a mesma sequência de comandos do editor oficial e só foi conferida na V20 (BlackBox_020). Em outra
  versão a página não grava.

## O que tem na cadeia (OpenPocket Bass)

GATE, OITAVA, COMP, EQ, AMP (NAM) e CAB (IR, 2000 amostras: 45 ms). Delay, reverb, chorus, drives, wah e synth foram
tirados. Os botões MOD, DLY e REV abrem o EQ, a oitava e o compressor.

## AMP e CAB (OpenPocket Bass)

- **AMP é sempre NAM** (não há outros amps pra escolher): MODEL (a captura), GAIN (o quanto a captura é empurrada: mais
  saturação; metade da mudança de volume é compensada), BASS, MID (600 Hz), TREBLE e **LEVEL** (o volume do amp, ±12 dB).
  O LEVEL é o 6º ajuste: fica guardado no primeiro ajuste do bloco SFX, que não existe na cadeia de baixo.
- **CAB é sempre USER IR**: IR, LOW CUT, HIGH CUT, **MIX** e **LEVEL**. O MIX dosa só o gabinete: 100 é tudo IR, 0 é a
  saída do amp sem IR. O LEVEL é o volume do lado do IR (±12 dB), antes do MIX.
- AMP e CAB não têm a linha de modelo: a captura e o IR ficam no topo da página.

## Volume do preset na tela principal

Na tela principal, o knob para no **VOL** (ao lado do nome) antes do primeiro pedal. Com ele marcado, o PUSH entra no
ajuste (fica laranja), o knob muda o volume do preset (0,3 dB por passo, de −15 a +15 dB) e o PUSH confirma. Segure o
PUSH pra salvar no preset. O knob MASTER continua sendo o volume geral.

## Renomear o preset na pedaleira

Segure o **PUSH** (página SAVE) e aperte **FX**: abre o nome. O knob anda pelas 13 letras e pelo OK; o PUSH numa letra
deixa trocar (o knob muda a letra, o PUSH confirma); o PUSH no OK volta pro SAVE com o nome novo; o HOME cancela. Depois
o PUSH no SAVE grava o preset com o nome.
