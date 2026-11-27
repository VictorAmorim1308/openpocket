// SPDX-License-Identifier: GPL-3.0-only
// OpenPocket Bass: the web editor in Portuguese (Brazil). The editor's own code writes English; this file translates
// what reaches the page: text, and the placeholder, title and aria-label attributes, as they appear (a MutationObserver).
// Exact phrases first, then patterns for the phrases that carry a name or a number. What the pedal itself shows (block,
// type and setting names, preset names) stays as on the pedal. ?lang=en (or the EN button) keeps English.

const EXACT = {
  // ---- page, connection ----
  "OpenPocket Editor": "Editor OpenPocket", "Skip to the editor": "Pular para o editor", "Editor": "Editor",
  "Presets": "Presets", "Sound": "Som", "Models": "Modelos", "Not connected": "Desconectada", "Disconnected": "Desconectada",
  "The pedal stopped answering. Plug it back in: the editor reconnects by itself.": "A pedaleira parou de responder. Conecte de novo: o editor reconecta sozinho.",
  "Reconnect now": "Reconectar agora", "Import": "Importar", "Back up all": "Backup de todos", "All": "Todos", "Yours": "Seus",
  "Factory": "Fábrica", "Compare": "Comparar", "Revert": "Desfazer edições", "Save as…": "Salvar como…", "Save": "Salvar",
  "Drag a block to move it. Arrow keys choose a block, Space switches it, Alt + arrows move it.": "Arraste um bloco para mudá-lo de lugar. As setas escolhem o bloco, Espaço liga/desliga, Alt + setas movem.",
  "Off": "Desligado", "On": "Ligado", "Models and cabinets": "Modelos e gabinetes", "OpenPocket installer": "Instalador OpenPocket",
  "Sections": "Seções", "DSP load": "Carga do DSP", "DSP load details": "Detalhes da carga do DSP", "Connection": "Conexão",
  "Import presets from a file": "Importar presets de um arquivo", "Save all 52 presets in one file": "Salvar os 52 presets num arquivo",
  "Search presets, amps, effects": "Buscar presets, amps, efeitos", "Search presets": "Buscar presets", "Show": "Mostrar",
  "Preset name": "Nome do preset", "Hear the saved preset, then come back to your changes": "Ouvir o preset salvo e depois voltar às suas mudanças",
  "Drop the changes and load the saved preset again": "Descartar as mudanças e carregar o preset salvo de novo", "More for this preset": "Mais opções do preset",
  "Signal chain, in the order the sound passes through": "Cadeia de sinal, na ordem em que o som passa",
  "Move this block earlier in the chain": "Mover o bloco para antes na cadeia", "Move this block later in the chain": "Mover o bloco para depois na cadeia",
  "Previous type": "Tipo anterior", "Type": "Tipo", "Next type": "Próximo tipo",
  "The pedal is back.": "A pedaleira voltou.", "Connect your pedal": "Conecte sua pedaleira", "Plug it in.": "Conecte.",
  "Pocket X or BlackBox, with a USB-C cable that carries data.": "Pocket X ou BlackBox, com um cabo USB-C que passe dados.",
  "Press Connect.": "Clique em Conectar.", "The browser asks to use MIDI devices: allow it.": "O navegador pede para usar dispositivos MIDI: permita.",
  "Play.": "Toque.", "Every change here sounds at once, and what you change on the pedal shows here.": "Toda mudança aqui soa na hora, e o que você muda na pedaleira aparece aqui.",
  "Connect": "Conectar", "The pedal needs OpenPocket v32 or newer.": "A pedaleira precisa do OpenPocket v32 ou mais novo.",
  "Install or update it": "Instale ou atualize", "first if it runs something older.": "antes, se ela tiver algo mais antigo.",
  "Browse, rename, reorder, copy and back up the 52 presets.": "Veja, renomeie, reordene, copie e faça backup dos 52 presets.",
  "The signal chain and every block's settings, live.": "A cadeia de sinal e os ajustes de cada bloco, ao vivo.",
  "Amp captures from TONE3000 or your files, and cabinet IRs.": "Capturas de amp (NAM) e IRs de gabinete: teste antes de gravar.",
  "The pedal's control numbers, and your own controller.": "Os números de controle da pedaleira e o seu controlador.",
  "This browser cannot reach the pedal": "Este navegador não alcança a pedaleira",
  "The editor talks to the pedal over Web MIDI, which this browser does not have. Open this page in": "O editor fala com a pedaleira por Web MIDI, que este navegador não tem. Abra esta página no",
  "Chrome or Edge": "Chrome ou Edge", "on a computer or an Android phone.": "num computador ou celular Android.",
  "Safari and browsers on iPhone and iPad have no Web MIDI.": "Safari e os navegadores de iPhone e iPad não têm Web MIDI.",
  "Back to the installer": "Voltar ao instalador", "MIDI access was refused": "O acesso MIDI foi negado",
  "The browser did not let this page use MIDI devices, so it cannot see the pedal.": "O navegador não deixou esta página usar dispositivos MIDI, então ela não vê a pedaleira.",
  "Click the icon left of the address, then": "Clique no ícone à esquerda do endereço e depois em", "Site settings": "Configurações do site",
  "Set": "Ponha", "MIDI devices": "Dispositivos MIDI", "(\"control and reprogram\") to Allow.": "(\"controlar e reprogramar\") em Permitir.",
  "Come back and try again.": "Volte e tente de novo.", "Try again": "Tentar de novo", "Looking for the pedal…": "Procurando a pedaleira…",
  "If the browser asks to use MIDI devices, allow it.": "Se o navegador pedir para usar dispositivos MIDI, permita.",
  "No pedal answered": "Nenhuma pedaleira respondeu", "The browser sees no MIDI device at all.": "O navegador não vê nenhum dispositivo MIDI.",
  "Is the pedal switched on and past its start screen?": "A pedaleira está ligada e já passou da tela de início?",
  "Does the cable carry data? Many USB-C cables only charge.": "O cabo passa dados? Muitos cabos USB-C só carregam.",
  "Does it run OpenPocket v32 or newer?": "Ela tem o OpenPocket v32 ou mais novo?",
  "Is another tab or program using it (the installer, a DAW)? Close that first.": "Outra aba ou programa está usando a pedaleira (o instalador, uma DAW)? Feche antes.",
  "The editor also tries again by itself as soon as a device is plugged in.": "O editor também tenta sozinho assim que um dispositivo é conectado.",
  "Reading the pedal…": "Lendo a pedaleira…", "Its blocks, types and settings are read once per firmware version, then remembered.": "Os blocos, tipos e ajustes são lidos uma vez por versão do firmware e depois lembrados.",
  "DSP overload: the pedal bypasses the effects": "DSP sobrecarregado: a pedaleira desliga os efeitos", "Room to spare.": "Folga de sobra.",
  "Getting full: another heavy block may not fit.": "Ficando cheio: outro bloco pesado pode não caber.",
  "Near the limit. One more block can push the pedal into its overload guard.": "Perto do limite. Mais um bloco pode ativar a proteção de sobrecarga.",
  "Overload guard: the pedal passes the guitar through without effects. Switch a block off, or choose a lighter type.": "Proteção de sobrecarga: a pedaleira passa o som sem efeitos. Desligue um bloco ou escolha um tipo mais leve.",
  "Now": "Agora", "Peak, this preset": "Pico, neste preset", "Peak since power-on": "Pico desde que ligou", "Late audio blocks": "Blocos de áudio atrasados",
  "Blocks at the clipper": "Blocos no limitador", "NAM network": "Rede NAM", "Where it goes": "Para onde vai", "Firmware": "Firmware", "Port": "Porta",
  "Keys": "Atalhos", "previous, next preset": "preset anterior, próximo", "Installer and updates": "Instalador e atualizações",
  // ---- shared ----
  "Dismiss": "Fechar", "Cancel": "Cancelar", "Rename": "Renomear", "Search": "Buscar", "Choose one.": "Escolha um.", "Choose": "Escolher",
  "Files": "Arquivos", "These files are not of the kind this panel takes.": "Estes arquivos não são do tipo que este painel aceita.",
  // ---- sound ----
  "Hearing the saved preset": "Ouvindo o preset salvo", "Edited": "Editado", "Back to my changes": "Voltar às minhas mudanças",
  "This is the preset as it is saved. Press “Back to my changes” to go on editing.": "Este é o preset como está salvo. Clique em “Voltar às minhas mudanças” para continuar editando.",
  "Hold a block, then drag it to move it. The arrows in the panel below move it too.": "Segure um bloco e arraste para mudá-lo de lugar. As setas no painel abaixo também movem.",
  "Your captures": "Suas capturas", "Your IRs": "Seus IRs", "This type has no settings.": "Este tipo não tem ajustes.",
  "The EQ's curve": "A curva do EQ", "Get or manage captures": "Carregar ou gerenciar capturas", "Load or manage IRs": "Carregar ou gerenciar IRs",
  "Compare with the saved preset": "Comparar com o preset salvo", "Nothing was changed.": "Nada foi mudado.", "Copy to…": "Copiar para…",
  "Move to…": "Mover para…", "Export": "Exportar", "Back to factory": "Voltar ao de fábrica", "This is already the factory preset.": "Este já é o preset de fábrica.",
  // ---- presets ----
  "Save, then load": "Salvar e carregar", "Drop the changes": "Descartar as mudanças", "Undo": "Desfazer", "Saving": "Salvando",
  "This is the preset itself.": "Este é o próprio preset.", "Choose the preset number it is saved under.": "Escolha o número do preset onde ele é salvo.",
  "Its own number: the same as Save.": "O próprio número: o mesmo que Salvar.", "Save as": "Salvar como",
  "The copy takes the place of the preset you choose.": "A cópia fica no lugar do preset que você escolher.", "Copy to": "Copiar para", "Copying": "Copiando",
  "Moving": "Movendo", "It takes that place; the presets between move by one. Nothing is replaced.": "Ele ocupa esse lugar; os presets entre eles andam uma posição. Nada é substituído.",
  "Name (13 characters at most)": "Nome (no máximo 13 caracteres)", "Renaming": "Renomeando", "Restoring": "Restaurando",
  "Backing up the presets": "Fazendo backup dos presets", "Import these presets?": "Importar estes presets?", "Importing": "Importando",
  "Replace the current sound?": "Substituir o som atual?", "Replace the sound": "Substituir o som", "The pedal refused this preset.": "A pedaleira recusou este preset.",
  "Load": "Carregar", "Rename…": "Renomear…", "Move up": "Subir", "Move down": "Descer", "Edited, not saved": "Editado, não salvo",
  "No preset of yours yet. Change a sound and press Save: it shows up here.": "Nenhum preset seu ainda. Mude um som e clique em Salvar: ele aparece aqui.",
  "Every preset is one of yours.": "Todos os presets são seus.",
  // ---- models ----
  "Amp captures": "Capturas de amp", "Cabinet IRs": "IRs de gabinete",
  "The AMP block's NAM type plays these neural captures. The pedal runs A2 captures (A2-Lite): on TONE3000, pick the A2 version.": "O bloco AMP toca estas capturas NAM. A pedaleira roda capturas A2 (A2-Lite): no TONE3000, escolha a versão A2.",
  "The CAB block's USER IR type plays these impulse responses. The pedal keeps the first 45 ms of a WAV file, at 44.1 kHz, and sets its level like the other cabinets.": "O bloco CAB toca estes IRs. A pedaleira guarda os primeiros 45 ms do arquivo WAV, em 44,1 kHz, e acerta o volume como o dos outros gabinetes.",
  "Reading the presets": "Lendo os presets", "Waiting": "Esperando", "Measuring its level": "Medindo o volume", "Reading": "Lendo",
  "In the library: the pedal's slots are full": "Na biblioteca: os slots da pedaleira estão cheios", "Empty": "Vazio", "No saved preset uses this slot.": "Nenhum preset salvo usa este slot.",
  "No saved preset uses it.": "Nenhum preset salvo usa.", "It stays in the library on this computer: you can send it back later.": "Fica na biblioteca deste computador: dá para mandar de novo depois.",
  "It is not in this browser's library: to get it back you need its file again.": "Não está na biblioteca deste navegador: para tê-lo de volta você precisa do arquivo.",
  "Remove from the pedal": "Tirar da pedaleira", "Name on the pedal (11 characters at most)": "Nome na pedaleira (no máximo 11 caracteres)",
  "The pedal shows this name in the block's settings. Presets keep using the slot.": "A pedaleira mostra este nome no bloco. Os presets continuam usando o slot.",
  "Presets that use it follow it to its new slot.": "Os presets que usam acompanham para o novo slot.", "It is here now.": "Já está aqui.",
  "Not in this browser's library: it cannot change places.": "Não está na biblioteca deste navegador: não dá para trocar de lugar.",
  "No saved preset is affected.": "Nenhum preset salvo é afetado.",
  "It is removed from this computer's library. What is on the pedal is not touched.": "Sai da biblioteca deste computador. O que está na pedaleira não muda.",
  "Delete": "Apagar", "Playing on the pedal": "Tocando na pedaleira", "Not playing": "Sem tocar",
  "This pedal's firmware cannot play a file before it is saved. Update it from the installer to try files first.": "O firmware desta pedaleira não toca um arquivo antes de gravar. Atualize pelo instalador para poder testar antes.",
  "Name on the pedal": "Nome na pedaleira", "Save into": "Gravar no slot", "Save to the pedal": "Gravar na pedaleira", "Drop it": "Descartar",
  "Not used": "Sem uso", "The presets that use it": "Os presets que usam", "Update it from the installer": "Atualize pelo instalador",
  "Add several": "Vários de uma vez", "Clear this list": "Limpar esta lista", "Move to another slot…": "Mover para outro slot…",
  "Not in this browser's library: load its file again to move it.": "Não está na biblioteca deste navegador: carregue o arquivo de novo para mover.",
  "Replace with a file…": "Substituir por um arquivo…", "All empty": "Todos vazios",
  "Built into the firmware: they need no slot and cannot be changed.": "Vêm no firmware: não usam slot e não mudam.",
  "Library on this computer": "Biblioteca deste computador",
  "This browser does not let the page keep files: the library lasts until the page is closed.": "Este navegador não deixa a página guardar arquivos: a biblioteca dura até fechar a página.",
  "Try": "Testar", "Hear it on the pedal before it takes a slot": "Ouvir na pedaleira antes de ocupar um slot", "Send to the pedal": "Mandar para a pedaleira",
  "TONE3000 browsing is not set up in this copy of the editor. Download the A2 capture from tone3000.com and add the .nam file here.": "A busca no TONE3000 não está configurada nesta cópia do editor. Baixe a captura A2 em tone3000.com e carregue o arquivo .nam aqui.",
  "Tone": "Timbre", "Where the capture goes": "Para onde vai a captura", "Library only (every slot is taken)": "Só na biblioteca (todos os slots ocupados)",
  "Library only, not the pedal": "Só na biblioteca, não na pedaleira", "Reading its captures…": "Lendo as capturas…",
  "This tone has no A2 capture, and the pedal plays A2 only. Choose another tone.": "Este timbre não tem captura A2, e a pedaleira só toca A2. Escolha outro.",
  "Into": "Para", "Get": "Pegar",
  "Amp, pedal and full-rig captures shared by players, free to use. OpenPocket plays their NAM A2 captures.": "Capturas de amp, pedal e rig completo compartilhadas por músicos, de graça. O OpenPocket toca as capturas NAM A2.",
  "TONE3000 opens in its own window: sign in there, search and listen, and choose a tone. It comes back here with its captures, ready for a slot.": "O TONE3000 abre numa janela própria: entre, busque, ouça e escolha um timbre. Ele volta aqui com as capturas, prontas para um slot.",
  "Waiting for the TONE3000 window…": "Esperando a janela do TONE3000…", "Browse TONE3000": "Buscar no TONE3000",
  "The sign-in lasts while this page is open and is not stored. The captures you get are kept in the library on this computer.": "O login dura enquanto a página estiver aberta e não é guardado. As capturas ficam na biblioteca deste computador.",
  "Signed in to TONE3000": "Conectado ao TONE3000", "Sign out": "Sair", "Search all of TONE3000": "Buscar em todo o TONE3000", "TONE3000 lists": "Listas do TONE3000",
  "This list could not be read. Search all of TONE3000 instead.": "Não deu para ler esta lista. Busque em todo o TONE3000.",
  "Favorites": "Favoritos", "No favorites yet. Mark tones with the heart on TONE3000 and they show up here.": "Nenhum favorito ainda. Marque timbres com o coração no TONE3000 e eles aparecem aqui.",
  "Downloaded": "Baixados", "Nothing downloaded with this account yet.": "Nada baixado com esta conta ainda.", "Trending": "Em alta",
  "TONE3000 lists nothing as trending right now.": "O TONE3000 não tem nada em alta agora.", "Latest": "Recentes", "TONE3000 lists nothing new right now.": "O TONE3000 não tem nada novo agora.",
  "the browser blocked the TONE3000 window: allow pop-ups for this page": "o navegador bloqueou a janela do TONE3000: permita pop-ups nesta página",
  "This file is not in this browser's library, and the pedal cannot send it back. Load its file again, then move it.": "Este arquivo não está na biblioteca deste navegador, e a pedaleira não consegue devolvê-lo. Carregue o arquivo de novo e depois mova.",
  "The other slot holds a file that is not in this browser's library, so the two cannot change places.": "O outro slot tem um arquivo que não está na biblioteca deste navegador, então os dois não podem trocar de lugar.",
  // ---- MIDI ----
  "Next preset": "Próximo preset", "Previous preset": "Preset anterior", "MASTER volume": "Volume MASTER", "MIDI device": "Dispositivo MIDI",
  "Controller": "Controlador", "Program Change is switched off below": "Program Change está desligado abaixo",
  "not learned: click a control below, then move this one": "não aprendido: clique num controle abaixo e mexa neste", "Pedal": "Pedaleira",
  "On: the computer records and plays": "Ligado: o computador grava e toca", "On: the computer records": "Ligado: o computador grava", "On: the computer plays": "Ligado: o computador toca",
  "The sound stops for a moment while the pedal saves the setting, then the pedal leaves USB for a second and the editor connects again.": "O som para um instante enquanto a pedaleira salva o ajuste; depois ela sai do USB por um segundo e o editor reconecta.",
  "Turn on": "Ligar", "Turn off": "Desligar", "The pedal could not save the setting.": "A pedaleira não conseguiu salvar o ajuste.",
  "USB MIDI port": "Porta USB MIDI", "Editor protocol": "Protocolo do editor", "Controllers": "Controladores",
  "None. Plug a MIDI controller into this computer and it shows up here.": "Nenhum. Conecte um controlador MIDI neste computador e ele aparece aqui.",
  "USB audio": "Áudio USB",
  "With USB audio on, the pedal is also a USB sound card, with no driver to install (stereo, 16 bit, 44.1 kHz). The computer records the pedal's sound, and what the computer plays comes out of the pedal's output.": "Com o áudio USB ligado, a pedaleira também é uma placa de som USB, sem driver (estéreo, 16 bits, 44,1 kHz). O computador grava o som da pedaleira, e o que o computador toca sai na saída da pedaleira.",
  "The pedal's own MIDI settings are on the pedal:": "Os ajustes MIDI da pedaleira ficam na pedaleira:", "hold FX for 0.8 s": "segure FX por 0,8 s",
  "for CHANNEL (OMNI or 1 to 16), SEND and CLOCK > DELAY. The editor cannot read or change them.": "para CHANNEL (OMNI ou 1 a 16), SEND e CLOCK > DELAY. O editor não lê nem muda esses ajustes.",
  "The MIDI reference": "A referência MIDI", "Waiting for a control. Click again to stop.": "Esperando um controle. Clique de novo para parar.",
  "Move a control…": "Mexa num controle…", "Where a learned control works": "Onde um controle aprendido funciona", "Press a switch…": "Aperte um botão…",
  "On / off": "Liga / desliga", "Controls": "Controles", "Straight to the pedal.": "Direto na pedaleira.",
  "A DAW or a controller that reaches the pedal's USB port uses the CC numbers below: they are fixed in the firmware. The names are this preset's: a setting's meaning follows the block's type.": "Uma DAW ou controlador ligado na porta USB da pedaleira usa os números CC abaixo: eles são fixos no firmware. Os nomes são os deste preset: o significado de um ajuste segue o tipo do bloco.",
  "Through this page.": "Por esta página.",
  "For a controller plugged into this computer: click a control below, then move a knob or press a switch on the controller. The page passes it on while it is open.": "Para um controlador ligado neste computador: clique num controle abaixo e mexa num knob ou aperte um botão do controlador. A página repassa enquanto estiver aberta.",
  "A control I teach works": "Um controle que eu ensino funciona", "Program Change loads presets": "Program Change carrega presets", "Block": "Bloco", "Switch": "Botão",
  "CC 7 is the MASTER volume. Values: a setting is 0 to 127 for 0 to 100; a switch is on from 64; a flip number acts on value 0, so a footswitch that sends 127 then 0 flips once.": "O CC 7 é o volume MASTER. Valores: um ajuste vai de 0 a 127 para 0 a 100; um botão liga a partir de 64; um número de alternar age no valor 0, então um footswitch que manda 127 e depois 0 alterna uma vez.",
  "Learned controls": "Controles aprendidos", "Every preset": "Todos os presets", "How the switch acts": "Como o botão age", "Each press flips it": "Cada toque alterna",
  "On while the value is 64 or more": "Ligado enquanto o valor for 64 ou mais",
  "None yet. A learned control is kept in this browser and works while this page is open and connected.": "Nenhum ainda. Um controle aprendido fica guardado neste navegador e funciona com esta página aberta e conectada.",
  "Monitor": "Monitor", "Clear": "Limpar",
  "Nothing received yet. Messages from a controller show here, and the pedal's own when a setting is changed on it (with SEND on).": "Nada recebido ainda. As mensagens de um controlador aparecem aqui, e as da pedaleira quando um ajuste muda nela (com SEND ligado).",
  // ---- the installer link, the language switch ----
  "English": "English", "in every preset": "em todos os presets",
};

// phrases with a name or a number in them: [English pattern, Portuguese]
const PAT = [
  [/^(\d+) of (\d+) in the chain$/, "$1 de $2 na cadeia"],
  [/^(\d+) of (\d+) slots used$/, "$1 de $2 slots usados"],
  [/^Yours (\d+)$/, "Seus $1"], [/^Factory (\d+)$/, "Fábrica $1"],
  [/^(.+) type$/, "tipo $1"],
  [/^(.+) (down|up) one step$/, (m, a, d) => `${a}: ${d === "down" ? "menos" : "mais"} um passo`],
  [/^(\S+) is empty: this preset plays without (an amp|a cabinet)\. Choose another (capture|IR) above, or load one into (\S+)\.$/,
    (m, s, w, t, s2) => `${s} está vazio: este preset toca sem ${w === "an amp" ? "amp" : "gabinete"}. Escolha outro(a) ${t === "capture" ? "captura" : "IR"} acima, ou carregue um(a) em ${s2}.`],
  [/^From the file (.+)\.$/, "Do arquivo $1."],
  [/^Wait for "(.+)" to finish\.$/, "Espere \"$1\" terminar."],
  [/^(.+) failed: (.+)\.$/, "$1 falhou: $2."],
  [/^The pedal did not answer \((.+)\)\.$/, "A pedaleira não respondeu ($1)."],
  [/^The pedal stopped answering while it was read \((.+)\)\. Check the cable and try again\.$/, "A pedaleira parou de responder durante a leitura ($1). Confira o cabo e tente de novo."],
  [/^Save the changes to (.+)\?$/, "Salvar as mudanças em $1?"],
  [/^(.+) has changes that are not saved\. Loading (.+) drops them\.$/, "$1 tem mudanças não salvas. Carregar $2 descarta essas mudanças."],
  [/^Undoing (.+)$/, "Desfazendo $1"], [/^Undone: (.+)\.$/, "Desfeito: $1."],
  [/^Saved as (.+)\.$/, "Salvo como $1."], [/^Back to the saved (.+)\.$/, "De volta ao $1 salvo."],
  [/^Save (.+) as…$/, "Salvar $1 como…"], [/^Save as (\d+)$/, "Salvar como $1"],
  [/^Copy (.+) to…$/, "Copiar $1 para…"], [/^(.+) is also preset (\d+) now\.$/, "$1 agora também é o preset $2."],
  [/^(.+) is preset (\d+) now\.$/, "$1 agora é o preset $2."], [/^Move (.+) to…$/, "Mover $1 para…"], [/^Move to (.+)$/, "Mover para $1"],
  [/^Rename (.+)$/, "Renomear $1"], [/^Preset (\d+) is (.+) now\.$/, "O preset $1 agora é $2."],
  [/^Put (\d+) back to its factory preset\?$/, "Voltar o $1 ao preset de fábrica?"],
  [/^Your preset (.+) leaves preset (\d+)\. Undo brings it back\.$/, "Seu preset $1 sai do $2. Desfazer traz de volta."],
  [/^Preset (\d+) is the factory (.+) again\.$/, "O preset $1 é de novo o de fábrica $2."],
  [/^(.+) exported(, with its unsaved changes)?\.$/, (m, a, b) => `${a} exportado${b ? ", com as mudanças não salvas" : ""}.`],
  [/^All (\d+) presets are in one file\. Import it to bring them back\.$/, "Os $1 presets estão num arquivo. Importe para trazê-los de volta."],
  [/^(\d+) holds your preset (.+)\. It is replaced; Undo brings it back\.$/, "O $1 tem seu preset $2. Ele é substituído; Desfazer traz de volta."],
  [/^Replace (\d+) (.+)$/, "Substituir $1 $2"],
  [/^(.+) is playing now\. Save it to keep it\.(.*)$/, "$1 está tocando agora. Salve para manter.$2"],
  [/^No preset matches "(.+)"\.$/, "Nenhum preset bate com \"$1\"."],
  [/^Actions for (.+)$/, "Ações para $1"],
  // models
  [/^Load a \.(nam|wav) file to try$/, "Carregar arquivo .$1 para testar"],
  [/^Several \.(nam|wav) files straight into the free slots, without trying them$/, "Vários arquivos .$1 direto nos slots livres, sem testar"],
  [/^Loading (.+) to try$/, "Carregando $1 para testar"],
  [/^Trying (.+)$/, "Testando $1"],
  [/^(.+) plays on the pedal now\. Try it, then save it or drop it\.$/, "$1 está tocando na pedaleira. Teste e depois grave ou descarte."],
  [/^(.+) is ready to save \(this firmware cannot play it before\)\.$/, "$1 está pronto para gravar (este firmware não toca antes)."],
  [/^Play your bass: the pedal plays it in (\S+) \(nothing written yet\)\. The (AMP|CAB) block's settings below change the sound live\.$/,
    "Toque seu baixo: a pedaleira toca no $1 (nada gravado ainda). Os ajustes do bloco $2 abaixo mudam o som ao vivo."],
  [/^Saving (.+) in (\S+)$/, "Gravando $1 no $2"],
  [/^(.+) is in (\S+)\. The sound playing now uses it: save the preset to keep it that way\.$/, "$1 está no $2. O som atual já usa: salve o preset para manter assim."],
  [/^Replace (.+) in (\S+)\?$/, "Substituir $1 no $2?"], [/^Replace (\S+)$/, "Substituir $1"],
  [/^(.+) leaves the pedal\. No saved preset uses it\.$/, "$1 sai da pedaleira. Nenhum preset salvo usa."],
  [/^(\S+)  (.+) \(replace\)$/, "$1  $2 (substituir)"], [/^(\S+)  empty$/, "$1  vazio"], [/^(\S+)  (.+)$/, "$1  $2"],
  [/^Adding (\d+) (captures|IRs)$/, (m, n, k) => `Adicionando ${n} ${k === "captures" ? "capturas" : "IRs"}`],
  [/^Adding (.+)$/, "Adicionando $1"], [/^Sending to (\S+)$/, "Mandando para $1"], [/^Sending (.+) to (\S+?)…?$/, "Mandando $1 para $2"],
  [/^In (\S+) as (.+)$/, "No $1 como $2"],
  [/^(\d+) (capture|captures|IR|IRs) on the pedal(.*)$/, (m, n, k, r) => `${n} ${/^cap/.test(k) ? (n === "1" ? "captura" : "capturas") : (n === "1" ? "IR" : "IRs")} na pedaleira${r}`],
  [/^(.+) is in (\S+)\. Choose it in the (AMP|CAB) block\.$/, "$1 está no $2. Escolha no bloco $3."],
  [/^Put (.+) in which slot\?$/, "Pôr $1 em qual slot?"],
  [/^Every slot is taken: (.+) takes the place of the (capture|IR) you choose\.$/, (m, a, k) => `Todos os slots estão ocupados: ${a} fica no lugar ${k === "capture" ? "da captura" : "do IR"} que você escolher.`],
  [/^(\d+) presets? \((.+)\) will play (.+) instead\.$/, "$1 preset(s) ($2) vão tocar $3 no lugar."],
  [/^Remove (.+) from (\S+)\?$/, "Tirar $1 do $2?"], [/^Emptying (\S+)$/, "Esvaziando $1"], [/^(\S+) is empty\.$/, "$1 está vazio."],
  [/^Renaming (.+)$/, "Renomeando $1"], [/^(\S+) is (.+) now\.$/, "$1 agora é $2."], [/^Moving (.+)$/, "Movendo $1"],
  [/^(.+) is in (\S+) now\.(.*)$/, "$1 está no $2 agora.$3"],
  [/^Delete (.+) from the library\?$/, "Apagar $1 da biblioteca?"], [/^Delete (.+) from the library$/, "Apagar $1 da biblioteca"],
  [/^(\d+) presets?$/, (m, n) => `${n} preset${n === "1" ? "" : "s"}`],
  [/^Load a file into (\S+)(.*)$/, "Carregar um arquivo no $1$2"],
  [/^(\d+) factory models$/, "$1 modelos de fábrica"],
  [/^Every (capture|IR) you add is kept in this browser, so it can move between slots, be renamed, or come back later\. The pedal itself cannot send one back\.$/,
    (m, k) => `Cada ${k === "capture" ? "captura" : "IR"} que você carrega fica guardado(a) neste navegador, para mudar de slot, renomear ou voltar depois. A pedaleira não consegue devolver arquivos.`],
  [/^All (.+) in the library (is|are) on the pedal\.$/, "Tudo o que está na biblioteca ($1) está na pedaleira."],
  [/^Nothing yet\. Add (\.nam|\.wav) files, or drop them on this panel\.$/, "Nada ainda. Carregue arquivos $1 ou solte-os neste painel."],
  [/^This pedal's firmware \((.+)\) has no (captures|IRs) yet\.$/, "O firmware desta pedaleira ($1) ainda não tem $2."],
  [/^Getting (.+) from TONE3000$/, "Pegando $1 do TONE3000"], [/^Downloading (.+)…$/, "Baixando $1…"], [/^Measuring the level of (.+)…$/, "Medindo o volume de $1…"],
  [/^Next free slot \((\S+)\)$/, "Próximo slot livre ($1)"], [/^(\S+) (.+): replace$/, "$1 $2: substituir"],
  [/^Get all (\d+) into free slots$/, "Pegar as $1 nos slots livres"], [/^Get (.+)$/, "Pegar $1"], [/^Open on TONE3000$/, "Abrir no TONE3000"],
  [/^level ([+-][\d.]+) dB$/, "volume $1 dB"],
  // midi
  [/^(.+) on \/ off$/, "$1 liga / desliga"], [/^Turn USB audio (on|off)\?$/, (m, a) => `${a === "on" ? "Ligar" : "Desligar"} o áudio USB?`],
  [/^USB audio is (on|off)\. The pedal leaves USB for a second; the editor connects again by itself\.$/, (m, a) => `Áudio USB ${a === "on" ? "ligado" : "desligado"}. A pedaleira sai do USB por um segundo; o editor reconecta sozinho.`],
  [/^Not changed: (.+)$/, "Não mudou: $1"], [/^Every preset$/, "Todos os presets"], [/^Only (.+)$/, "Só no $1"], [/^only in (.+)$/, "só no $1"],
  [/^version (\d+)$/, "versão $1"], [/^Settings in (.+)$/, "Ajustes em $1"], [/^learned: (.+)$/, "aprendido: $1"], [/^Forget (.+) for (.+)$/, "Esquecer $1 para $2"],
];

const ATTRS = ["placeholder", "title", "aria-label"];
let on = true;
const tr = (s) => {
  if (!on || !s) return s;
  const m = /^(\s*)([\s\S]*?)(\s*)$/.exec(s), t = m[2];
  if (!t || !/[a-z]/.test(t)) return s;
  if (Object.prototype.hasOwnProperty.call(EXACT, t)) return m[1] + EXACT[t] + m[3];
  for (const [re, to] of PAT) if (re.test(t)) return m[1] + t.replace(re, to) + m[3];
  return s;
};
const SKIP = new Set(["SCRIPT", "STYLE", "TEXTAREA", "INPUT"]);
function walk(root) {
  if (root.nodeType === 3) { const v = tr(root.nodeValue); if (v !== root.nodeValue) root.nodeValue = v; return; }
  if (root.nodeType !== 1) return;
  if (root.closest && root.closest("[data-notr]")) return;
  for (const a of ATTRS) if (root.hasAttribute(a)) { const o = root.getAttribute(a), v = tr(o); if (v !== o) root.setAttribute(a, v); }
  if (SKIP.has(root.tagName)) return;
  for (const c of root.childNodes) walk(c);
}
export const lang = () => (on ? "pt" : "en");
export function startLang() {
  const q = new URLSearchParams(location.search).get("lang");
  let pref = q || (() => { try { return localStorage.getItem("op-lang"); } catch (_) { return null; } })() || "pt";
  on = pref !== "en";
  document.documentElement.lang = on ? "pt-BR" : "en";
  // the switch, in the bar
  const end = document.querySelector(".bar-end");
  if (end && !document.getElementById("langsw")) {
    const b = document.createElement("button");
    b.type = "button"; b.id = "langsw"; b.className = "btn small"; b.dataset.notr = "1";
    b.textContent = on ? "EN" : "PT"; b.title = on ? "English" : "Português";
    b.onclick = () => { try { localStorage.setItem("op-lang", on ? "en" : "pt"); } catch (_) { } const u = new URL(location.href); u.searchParams.delete("lang"); location.href = u.href; };
    end.prepend(b);
  }
  if (!on) return;
  walk(document.body);
  document.title = tr(document.title);
  new MutationObserver((list) => {
    for (const r of list) {
      if (r.type === "characterData") { const v = tr(r.target.nodeValue); if (v !== r.target.nodeValue) r.target.nodeValue = v; }
      else if (r.type === "attributes") { const o = r.target.getAttribute(r.attributeName), v = tr(o); if (v !== o) r.target.setAttribute(r.attributeName, v); }
      else for (const n of r.addedNodes) walk(n);
    }
    const t = tr(document.title); if (t !== document.title) document.title = t;
  }).observe(document.body, { subtree: true, childList: true, characterData: true, attributes: true, attributeFilter: ATTRS });
}
