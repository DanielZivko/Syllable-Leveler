# Dependências e origem

`Source/ARAPlugin.h` deriva de `examples/Plugins/ARAPluginDemo.h` do JUCE 8.0.6.
O aviso ISC original foi preservado no cabeçalho. Modificações: motor de ganho por
segmento, interface de análise/A-B/intensidade, representação do mapa, serialização
do mapa, checagens adicionais e identificação própria do plugin.

JUCE: https://github.com/juce-framework/JUCE/tree/8.0.6
O restante do framework possui termos próprios (licença comercial ou AGPLv3),
que devem ser considerados antes de distribuir o produto. A licença ISC do exemplo
não se estende a todo o framework.

ARA SDK: https://github.com/Celemony/ARA_SDK
Publicado pela Celemony sob Apache-2.0. O CMake baixa as dependências; elas não estão
incluídas neste pacote de código-fonte. Os avisos originais permanecem nos repositórios.

Nenhuma licença de distribuição do código original deste projeto foi escolhida nesta
entrega. Este e um prototipo de desenvolvimento hospedado no repositorio do proprietario.
