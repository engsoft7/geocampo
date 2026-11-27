<p align="center">
  <img src="images/icons/geocampo_logo.svg" alt="GeoCampo" width="128"/>
</p>

# GeoCampo

Um SIG (sistema de informação geográfica) completo para Android, construído sobre o motor do QGIS.

O GeoCampo é um fork do [QField](https://github.com/opengisch/QField), desenvolvido pela
[OPENGIS.ch](https://opengis.ch) e colaboradores, que por sua vez usa as bibliotecas do
[QGIS](https://qgis.org). Por isso ele já abre os mesmos dados e projetos do QGIS desktop:
camadas vetoriais e raster, sistemas de coordenadas, simbologia, rótulos, formulários e
relações entre tabelas.

## O que o app já faz

Herdado do QField 4.3.8:

- Abre projetos do QGIS (`.qgs` / `.qgz`) com estilos, rótulos e formulários.
- Formatos vetoriais: GeoPackage, Shapefile, GeoJSON, KML/KMZ, GPX, SpatiaLite, CSV e
  tudo o que o GDAL/OGR suporta; também PostGIS, WFS e WMS/WMTS/XYZ.
- Rasters: GeoTIFF, COG, MBTiles e outros formatos do GDAL, além de tiles vetoriais.
- Reprojeção entre sistemas de coordenadas (PROJ), incluindo SIRGAS 2000 e UTM.
- GPS interno e receptores externos (Bluetooth, serial, NMEA, RTK via NTRIP).
- Coleta e edição de pontos, linhas e polígonos, com snapping, rastreamento (tracking) e
  ferramentas de geometria (dividir, unir, rotacionar, desfazer).
- Tabela de atributos, formulários, fotos/anexos, leitura de QR code e NFC.
- Medição de distâncias e áreas e busca de feições.
- Ferramentas de processamento do QGIS (buffer, recorte, simplificação e outras).
- Layouts de impressão em PDF, visualização 3D e suporte a plugins em QML.
- Uso totalmente offline.

## Roteiro rumo a um "QGIS completo" no celular

O QField foi pensado para trabalho de campo. Para se aproximar do QGIS desktop, os
próximos passos do GeoCampo são:

1. Painel de camadas com edição de estilo (cores, classificação, rótulos) no próprio app.
2. Criação de camadas e projetos novos sem precisar do QGIS desktop.
3. Caixa de ferramentas de processamento mais ampla, com histórico e modelos.
4. Editor de layouts de impressão.
5. Calculadora de campos e consultas por expressão.
6. Georreferenciador de imagens.
7. Tradução completa da interface para português do Brasil.

## Como baixar o APK

Cada push na branch `main` dispara o workflow **📱 Android** em
[Actions](../../actions). Ao terminar, o APK fica disponível como artefato
(`geocampo-...-android-arm64`), que funciona na grande maioria dos celulares atuais.

As versões estáveis ficam em [Releases](../../releases), com o APK pronto para baixar
direto no celular.

As compilações de teste da `main` são instaladas como **GeoCampo Dev**
(`io.github.engsoft7.geocampo_dev`) e as versões estáveis como **GeoCampo**
(`io.github.engsoft7.geocampo`), então as duas podem coexistir no mesmo aparelho.

## Como lançar uma versão

As releases são criadas automaticamente pelo GitHub Actions:

1. Altere o número no arquivo [`VERSION`](VERSION) (por exemplo, de `0.1.0` para `0.2.0`).
2. Opcional: escreva as notas da versão em `releases/v0.2.0.md`. Sem esse arquivo, o
   GitHub gera as notas a partir dos commits.
3. Faça push na `main`.

O workflow compila o APK, cria a tag `v0.2.0`, publica a release e anexa o APK. Se o
número no `VERSION` já tiver sido publicado, o push gera só o APK de teste.

Também dá para lançar sem mexer no arquivo: em [Actions](../../actions) →
**📱 Android** → **Run workflow**, informe a versão (ex.: `0.2.0`). O formato aceito é
`X.Y.Z` ou `X.Y.Z-rcN`.

### Tempo de compilação

As dependências (Qt, QGIS, GDAL, PROJ e outras) são compiladas com o vcpkg. O workflow
tenta primeiro reaproveitar os binários pré-compilados que o projeto QField publica e
guarda no cache do GitHub tudo o que precisar compilar. Se a primeira execução estourar o
limite de 6 horas do GitHub, basta executá-la de novo: ela continua de onde parou.

### Assinatura para a Play Store (opcional)

Sem configuração, o APK é assinado com uma chave de depuração: instala normalmente, mas
não pode ser publicado na Play Store. Para assinar com a sua chave, cadastre estes
secrets em *Settings → Secrets and variables → Actions*:

| Secret | Conteúdo |
|---|---|
| `ANDROID_KEYSTORE_BASE64` | O keystore `.p12` codificado em base64 |
| `ANDROID_KEY_ALIAS` | O alias da chave |
| `ANDROID_KEY_PASSWORD` | A senha da chave |
| `ANDROID_STORE_PASSWORD` | A senha do keystore |

## Compilar localmente

As instruções de compilação do QField continuam válidas, em [`doc/dev.md`](doc/dev.md). O nome,
o ícone e o identificador do pacote podem ser alterados pelas opções do CMake
`APP_NAME`, `APP_ICON`, `APP_PACKAGE_DOMAIN` e `APP_PACKAGE_NAME`.

## Créditos e licença

- [QField](https://github.com/opengisch/QField): © OPENGIS.ch e colaboradores.
- [QGIS](https://github.com/qgis/QGIS): © QGIS Development Team.

O GeoCampo é software livre, distribuído sob a
[GNU General Public License v2 ou posterior](LICENSE), a mesma licença do QField e do QGIS.
Quem distribuir o app, modificado ou não, deve disponibilizar o código-fonte nos mesmos
termos.

QField e QGIS são marcas de seus respectivos titulares. O GeoCampo é um projeto
independente e não é afiliado nem endossado por eles.
