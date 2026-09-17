# Syllable Leveler — protótipo 0.1

Projeto de nivelamento de voz por segmentos para Zivko.

**Esta entrega contém código-fonte. Não contém um Audio Unit compilado nem validado no Logic.**
O motor independente foi compilado e testado em Linux. A integração AU/ARA e a interface
foram escritas a partir do exemplo oficial do JUCE e ainda precisam de compilação no Mac,
validação pelo `auval` e teste no Logic Pro. O workflow incluído ainda não foi executado.

## Destino

- Logic Pro no macOS Big Sur: Audio Unit v2 (.component) com ARA.
- Deployment target: macOS 11.0; configuração universal Intel + Apple Silicon.
- Ambiente de referencia confirmado: Mac Intel, macOS Big Sur, Logic Pro 10.7.4.
- Neste Mac Intel nao e necessario Rosetta.
- Para ARA no Logic em Apple Silicon, a Apple orienta abrir o Logic usando Rosetta.
  [Fonte oficial](https://support.apple.com/en-us/102082).
- Esta primeira configuração gera AU. A base JUCE permite acrescentar VST3 futuramente;
  um VST3 não é o formato a instalar no Logic.

## O que foi implementado no código

1. Detector acústico usando energia por quadros de 10 ms, suavização, pausas e vales de energia.
2. Mapa com início/fim em amostras, nível RMS, pico e ganho por segmento.
3. Alvo calculado pela mediana dos níveis em dB dos segmentos — evita que um grito isolado
   desloque excessivamente o alvo. Não é uma medição de loudness percebido/LUFS.
4. Intensidade inicial de 50%, ganho máximo de +6 dB e redução máxima de −12 dB.
5. Limite adicional de aumento pela folga do pico amostrado até −1 dBFS.
6. Transição suave, com largura nominal de 12 ms dentro de cada borda. O ganho retorna
   a 0 dB nas bordas. Essa opção conservadora evita levar o aumento de um segmento para
   o vizinho mais alto; sua qualidade auditiva ainda precisa de avaliação.
7. Silêncios fora dos segmentos permanecem inalterados. Canais estéreo recebem o mesmo ganho.
8. Adaptador de reprodução ARA, mapa sobre a forma de onda, menu de intensidade e A/B.
9. Código de persistência do mapa no documento ARA, clonagem de modificações e invalidação
   quando o host avisa que o conteúdo da fonte mudou.
10. Configuração CMake e GitHub Actions para construir o AU no macOS.

## Limitações explícitas desta primeira versão

- **Não reconhece sílabas linguisticamente.** Os segmentos são estimativas acústicas.
  Vogais ligadas podem virar um único segmento; vibrato de amplitude intenso pode causar
  divisões indevidas. Ainda não há corpus de voz real nem estimativa de acurácia.
- Não reconhece respirações, sibilância ou ruído como categorias. Ruído abaixo do limiar
  é ignorado; respirações acima dele ainda podem receber ganho.
- A análise é acionada manualmente e síncrona na thread da interface. Use com o transporte
  parado; pode haver pausa da interface. O próximo passo é captura incremental e análise
  cancelável em segundo plano.
- Limite da análise: **arquivo-fonte inteiro de até 120 s e até 24 milhões de amostras por
  canal**. Recortar visualmente uma região longa na DAW não reduz o tamanho da fonte.
  Para o primeiro teste, use um arquivo curto consolidado/bounce de voz isolada.
- Fonte, sessão e instância do plugin devem usar a **mesma taxa de amostragem e quantidade
  de canais**. Ex.: arquivo mono de 48 kHz, sessão de 48 kHz, instância mono. Incompatibilidade
  não tem conversão automática; a reprodução ARA pode ficar silenciosa e a interface exibe aviso.
- Sem time stretch/Flex, mudanças de velocidade, edição manual das divisões, undo próprio,
  proteção true-peak, automação de parâmetros ou exportação independente nesta entrega.
- O teto por pico limita aumentos individuais; não corrige clipping já presente, picos
  interamostrais ou a soma de regiões sobrepostas. Não é um limiter final.
- O mapa pertence à modificação ARA: regiões que compartilham essa modificação compartilham
  o mapa. Não são criados cortes destrutivos nem automação de volume na pista do Logic.
- As leituras ARA usam a infraestrutura de buffering do exemplo JUCE. Ainda é necessário
  testar regiões adicionadas/removidas durante uma sessão, underruns e bounce offline.
- Sem aprovação de compatibilidade Big Sur, assinatura Developer ID ou notarização.

## Uso pretendido depois de compilar e validar

1. Abrir uma sessão de teste com voz isolada e curta, sem Flex.
2. Inserir **Zivko Audio > Syllable Leveler** no primeiro slot de Audio FX.
3. Parar a reprodução e abrir a interface.
4. Dar duplo clique na forma de onda para analisar a fonte.
5. As marcas mostram as divisões; quando há espaço, cada trecho mostra seu ganho em dB.
6. Outro duplo clique alterna original/processado. Botão direito permite reanalisar ou
   escolher intensidade de 25%, 50%, 75% ou 100%.
7. Usar +/− para zoom. Os rótulos de ganho representam a correção calculada, mesmo em ORIGINAL.

## Compilar

Repositorio: https://github.com/DanielZivko/Syllable-Leveler

O workflow do GitHub compila o projeto sem exigir ferramentas de desenvolvimento no Mac
de teste. O repositorio foi criado publico pelo proprietario. A primeira compilacao pode
revelar ajustes necessarios.

O workflow `.github/workflows/build.yml` testa o motor em Linux e tenta construir o AU
universal em macOS. Se terminar com sucesso, o artefato contém a pasta `.component` compactada.
O sucesso no CI não substitui o teste no Big Sur/Logic.

Para desenvolvimento local, são necessários Git, CMake 3.22+ e Xcode/Command Line Tools
compatíveis com o macOS usado para construir. Não é necessário atualizar seu Mac de teste
para construir no CI. A disponibilidade de SDKs e a compatibilidade do compilador local
ainda precisam ser verificadas no seu Mac.

```bash
bash scripts/build-mac.sh
```

Dependências buscadas pelo CMake:

- JUCE 8.0.6, tag fixada.
- ARA SDK, commit `de1ad0d1d23388047449ef4ff0f57122f218db96`, incluindo ARA_API e ARA_Library.

Não se instala nem se substitui nenhum plugin automaticamente. O resultado pretendido é:
`build/SyllableLeveler_artefacts/Release/AU/Syllable Leveler.component`.

## Testar somente o motor

Sem JUCE, ARA ou acesso à rede:

```bash
cmake -S . -B build-core -DSYLLABLE_BUILD_PLUGIN=OFF
cmake --build build-core
ctest --test-dir build-core --output-on-failure
```

Ou diretamente com um compilador C++17:

```bash
c++ -std=c++17 -I Source Tests/LevelingTests.cpp -o leveling_tests
./leveling_tests
```

Veja `docs/VALIDATION.md` para resultados e testes ainda pendentes.

## Próximas entregas

1. Executar a primeira compilação AU; corrigir eventuais erros de integração.
2. Validar descoberta/abertura ARA no Logic e reprodução sem processamento.
3. Testar mapa, ganho, salvar/reabrir e bounce com amostras de fala e canto.
4. Implementar análise assíncrona, seleção de região e feedback de progresso.
5. Permitir arrastar limites, dividir, unir, ajustar ganho individual e desfazer.
6. Refinar detector com exemplos anotados; distinguir respiração e sibilância.
7. Acrescentar conversão de taxas/canais e testes extensivos de host.

## Referências e licenças

- [ARA SDK](https://github.com/Celemony/ARA_SDK)
- [Exemplo oficial JUCE usado como base](https://github.com/juce-framework/JUCE/blob/8.0.6/examples/Plugins/ARAPluginDemo.h)
- [Apple: ARA e Rosetta](https://support.apple.com/en-us/102082)

O exemplo JUCE tem licença ISC e seu aviso está preservado em `Source/ARAPlugin.h`.
As dependências JUCE e ARA têm licenças próprias; ver `THIRD_PARTY_NOTICES.md`.
