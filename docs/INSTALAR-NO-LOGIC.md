# Teste no Logic Pro 10.7.4 — Mac Intel / Big Sur

Use somente um artefato de uma compilação com status verde no GitHub Actions.
O arquivo de código-fonte não é o plugin instalável.

1. Abra a execução da compilação no GitHub, entre em **Artifacts** e baixe
   **SyllableLeveler-AU-development**. É necessário estar conectado ao GitHub.
2. Descompacte os ZIPs até encontrar **Syllable Leveler.component**.
   A pasta `.component` deve permanecer inteira.
3. Feche o Logic. No Finder, escolha **Ir > Ir para a pasta** e cole:
   `~/Library/Audio/Plug-Ins/Components`
4. Copie **Syllable Leveler.component** para essa pasta.
5. Abra o Logic. Em **Logic Pro > Preferences > Plug-in Manager**, procure
   **Syllable Leveler**, fabricante **Zivko Audio**. Se necessário, use
   **Reset & Rescan Selection** apenas nesse plugin.
6. Crie uma sessão de teste com um arquivo curto de voz isolada, preferencialmente
   mono, de 10–30 segundos. Use a mesma taxa de amostragem do arquivo na sessão.
7. Insira a versão mono do plugin em **Audio FX > Audio Units > Zivko Audio**,
   no **primeiro slot** da pista. Para arquivo estéreo, use pista/instância estéreo.
8. Com a reprodução parada, abra o plugin e dê duplo clique na forma de onda.
   Outro duplo clique alterna original/processado. Botão direito muda a intensidade.
9. Comece com 25% ou 50%. Ouça o resultado e depois teste salvar/reabrir e bounce.

No Mac Intel não é necessário Rosetta.

Se o plugin não aparecer, a validação falhar ou o macOS bloquear a abertura,
mande a mensagem exata ou uma captura da tela. Não desative globalmente as
proteções do macOS. Esta é uma compilação de desenvolvimento, sem notarização.

Limitações atuais: detecção acústica aproximada, análise síncrona, arquivo-fonte
inteiro limitado a 120 s/24 milhões de amostras, sem Flex/time stretch, sem edição
manual das divisões e sem conversão de canais/taxa. A aprovação do auval em uma
máquina de compilação não garante compatibilidade com Big Sur/Logic 10.7.4.
