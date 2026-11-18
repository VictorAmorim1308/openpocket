# OpenPocket Bass (M-VAVE BlackBox)

Uma versão do OpenPocket pensada para baixo. A cadeia dos presets de fábrica é:

    PITCH (BASS OCTAVE)  ->  EQ (BASS PEQ)  ->  AMP (NAM)  ->  CAB (USER IR + BLEND)  ->  MOD (BASS CHORUS)

Os outros blocos continuam no firmware, só que desligados nos presets de baixo. O compressor, por exemplo, fica à mão no preset SLAP.

## O que mudou em relação ao OpenPocket

| Onde | O quê |
| --- | --- |
| EQ, tipo **BASS PEQ** | LOW (shelf em 80 Hz, ±12 dB) · MID FREQ (**40 Hz a 4 kHz**) · MID GAIN (±12 dB) · MID Q (0.4 a 6.0) · HIGH (shelf em 3,5 kHz, ±12 dB). O PARAMETRIC original só descia até 150 Hz. |
| CAB, tipo **USER IR** | Ganhou **BLEND** (0 = só amp+IR; 100 = só o limpo) e **CLN PHASE** (NORMAL / INVERT). O sinal limpo é pego **na entrada do amp**, ou seja, depois do EQ. |
| PITCH, tipo **BASS OCTAVE** | Oitavador analógico de uma nota só (divisor flip-flop): DRY · SUB −1 · UP +1 · TONE (60 a 600 Hz, filtro do sub) · TRACK (100 a 500 Hz; mais baixo deixa o rastreio mais firme nas cordas graves). O disparo tem histerese pra não "gaguejar" no fim da nota, e o filtro da oitava acima acompanha a nota tocada. Fica **antes do EQ**, onde rastreia o baixo limpo. |
| MOD, tipo **BASS CHORUS** | RATE · DEPTH · MIX · **X-OVER** (50 a 500 Hz). Abaixo do X-OVER o grave passa seco, no estilo dos chorus de baixo. |
| Afinador | Agora lê de **27,6 Hz a 551 Hz**: Si grave (30,9 Hz) do baixo de 5 cordas, Mi grave (41,2 Hz) e harmônicos na 12ª casa. O original começava em 61 Hz. |
| Presets | 01 DI LIMPO · 02 AMP 1 · 03 AMP 1 BLEND · 04 AMP 2 · 05 AMP 2 BLEND · 06 GROWL · 07 SLAP · 08 CHORUS · 09 AMP 3 · 10 AMP 4 · 11 OITAVA · 12–52 USER (EQ → U1 → IR1, tudo flat) |
| Tela de boot | Mostra `v42 BASS`. |

## Como usar o BLEND

1. Carregue uma captura NAM **A2** de amp de baixo no slot U1 e um IR no slot IR1, pelo editor web (painéis "NAM models" e "IRs").
2. Abra o preset **03 AMP 1 BLEND**. O IR tem LOW CUT em 80 Hz, então o grave vem do sinal limpo e o amp entra por cima.
3. Gire o **BLEND**. Se o grave **diminuir** quando o BLEND chega perto do meio, troque **CLN PHASE** para INVERT. Algumas capturas invertem a polaridade do grave. Nos 12 amps de fábrica, por exemplo, 8 pedem INVERT (veja `tests/engine/bass_test.c`, parte 3).
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
