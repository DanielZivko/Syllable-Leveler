# Validação — 17/09/2026

## Executado nesta entrega

Ambiente Linux, GCC 13.3, C++17. Compilação com `-Wall -Wextra -Wpedantic -Werror`
e instrumentação AddressSanitizer/UndefinedBehaviorSanitizer.

O LeakSanitizer não funciona sob a instrumentação do ambiente e foi desativado para a
execução (`ASAN_OPTIONS=detect_leaks=0`). Isso não é uma verificação de vazamentos.

Resultado: suíte do motor aprovada, sem erro reportado por ASan/UBSan.

| Cenário sintético | Resultado |
|---|---|
| Três eventos separados a 44,1/48/96 kHz | Três segmentos; quieto aumenta, alto diminui |
| Intensidade 50% | Diferença interna de 12,0412 dB caiu para 6,0206 dB |
| Intensidade zero | Saída idêntica amostra a amostra |
| Reprodução em blocos de 127 amostras | Idêntica ao processamento do arquivo inteiro |
| Silêncio e ruído abaixo de −48 dBFS | Sem segmentos; silêncio inalterado |
| Estéreo em oposição de fase | Não desaparece na análise; medição por energia dos canais |
| Vogal sintética sustentada | Um segmento |
| Vale de energia entre eventos ligados | Detectou divisão |
| Limites de ganho e folga de pico | Respeitados |
| Bordas do ganho | Unidade na borda; primeiro passo suavizado |
| NaN em áudio/parâmetros | Rejeitado |
| Buffers curtos/parciais | Sem segmentos fora dos limites |

Os números de redução de diferença referem-se ao interior dos eventos sintéticos,
excluindo transições. Não são uma avaliação de loudness percebido ou de inteligibilidade.

## Ainda não executado

- Compilação/link do AU/ARA e execução do workflow GitHub Actions.
- Validação Audio Unit `auval -v aufx Sylv Zivk`.
- Carregar no Logic/Big Sur em Intel e/ou Apple Silicon via Rosetta.
- Comparação original/processado em fala ou canto real.
- Persistência de documento ARA e recuperação de arquivos incompletos.
- Bounce offline, congelamento, regiões duplicadas, movimentação na timeline e loop.
- Profiler de tempo real, consumo de CPU/memória e testes de concorrência.

## Roteiro de aceite no Mac

1. Registrar versão exata do Logic, macOS e chip.
2. Compilar; verificar bundle AU, arquiteturas e deployment target.
3. Executar auval e registrar o resultado. Não considerar a aprovação como teste de ARA.
4. Abrir em sessão descartável, primeiro slot; verificar acesso ARA à fonte.
5. Antes de analisar, conferir que a reprodução coincide com a fonte.
6. Analisar um arquivo mono seco de 10–30 s na taxa da sessão; confirmar ausência de
   estouros e se a interface mostra divisões e ganhos.
7. Alternar A/B com transporte parado; ouvir consoantes, respirações e notas sustentadas.
8. Salvar, fechar e reabrir a sessão; confirmar que o mapa e a intensidade foram preservados.
9. Exportar bounce offline e comparar ao playback; testar mono e estéreo separadamente.
10. Anotar divisões erradas para melhorar o detector antes de ampliar o projeto.
