# DataSources do dash001.4

O JSON da board define **o que consultar** em `DataSources` e **onde mostrar**
em `Screens[].Objects`. Declarar uma fonte não cria objetos no ecrã.
Os tipos implementados são `Weather`, `HA` e `Ping`.

Os exemplos abaixo são fragmentos JSON válidos: os arrays de fontes entram em
`DataSources` e os arrays de objetos em `Screens[].Objects`. Ao combinar
exemplos, juntar os elementos num único array, sem repetir as chaves na raiz.
Os nomes e as maiúsculas das chaves devem seguir os exemplos.

## Estrutura de uma board

Exemplo completo e utilizável com um ping:

```json
{
  "TIMEZONE": "Europe/Lisbon",
  "touchEnable": true,
  "Log": {"Weather": false, "HA": false, "Ping": false},
  "DataSources": [
    {"Type": "Ping", "Id": "server1", "Host": "192.168.1.204", "Refresh": 10000}
  ],
  "Screens": [
    {
      "Number": 1,
      "Rotation": 1,
      "Invert": true,
      "Refresh": 1000,
      "Objects": [
        {"Type": "FillScreen", "Color": "Black"},
        {"Type": "DrawText", "X": 10, "Y": 10, "Text": "function:Clock", "Font": 4, "Color": "White"},
        {"Type": "DrawText", "X": 10, "Y": 100, "Text": "ping:server1", "Prefix": "192.168.1.204 ", "Font": 2, "Color": "White"}
      ]
    }
  ]
}
```

`DataSources[].Refresh` define o intervalo de consulta, em milissegundos;
`Screens[].Refresh` define o intervalo de desenho. Os pedidos e a execução das
outras fontes podem aumentar o intervalo real. O desenho lê os dados em cache.
As fontes são globais à board, mesmo quando se muda de ecrã.

As posições são em píxeis, a partir do canto superior esquerdo, sem escalamento
automático. `Rotation: 1` dá 320×240 nas amarelas/pink e 480×320 na black.
Os exemplos são peças de layout: ajustar posições ao juntá-los para evitar
sobreposição. O `simple.v4.json` atual já tem os pings no lugar dos sensores da sala.

## Fontes disponíveis no projeto

| Id | Tipo | Conteúdo | Referência usada no desenho |
| --- | --- | --- | --- |
| `weather_aveiro` | Weather | Meteorologia da localização definida no URL | `datasource:weather_aveiro:campo` |
| `doors` | HA | Seis portas | `DrawHASensor` com `entity` |
| `people` | HA | Quatro sensores de presença | `DrawHASensor` com `entity` |
| `sala` | HA | Temperatura, humidade e luminosidade | `entity:sensor...` em `DrawText` |
| `server1` | Ping | `192.168.1.204` | `ping:server1` |
| `server2` | Ping | `192.168.1.75` | `ping:server2` |
| `server3` | Ping | `192.168.1.79` | `ping:server3` |

A fonte `sala` está documentada para reutilização; foi retirada do
`simple.v4.json` a pedido do utilizador. Os nomes `Id` podem ser alterados,
desde que se atualizem as referências correspondentes.

## Weather — meteorologia

### Definir a fonte

```json
[
  {
    "Type": "Weather",
    "Id": "weather_aveiro",
    "Refresh": 600000,
    "url": "https://api.open-meteo.com/v1/forecast?latitude=41.343&longitude=-8.477&current=temperature_2m,relative_humidity_2m,pressure_msl,weather_code,wind_speed_10m",
    "Map": {
      "temperature": "current.temperature_2m",
      "humidity": "current.relative_humidity_2m",
      "pressure": "current.pressure_msl",
      "wind": "current.wind_speed_10m",
      "code": "current.weather_code"
    },
    "Codes": {
      "0": {"label": "Ceu limpo", "image": "sun"},
      "2": {"label": "Parcialmente nublado", "image": "cloud_sun"},
      "3": {"label": "Nublado", "image": "cloud"},
      "61": {"label": "Chuva fraca", "image": "rain_light"}
    }
  }
]
```

- `url`, com minúsculas, é o endereço do serviço. A localização é determinada
  pelas coordenadas, não pelo nome `Id`.
- `Map` associa os campos do dashboard a caminhos no JSON recebido, separados
  por pontos. O firmware não acrescenta unidades nem converte os valores.
- `Codes` associa o código meteorológico à descrição e ao nome de um ícone
  interno de `images.h`. O mapa acima é abreviado; copiar o mapa mais completo
  de `jsons/simple.v4.json` para cobrir os restantes estados.
- Um código sem correspondência deixa descrição e nome do ícone vazios.
  A propriedade `url` dentro de cada código, presente em alguns layouts,
  não é usada para descarregar imagens.

### Mostrar na board

```json
[
  {"Type": "DrawImage", "X": 10, "Y": 70, "Image": "datasource:weather_aveiro:code_image", "Zoom": 1, "Color": "White"},
  {"Type": "DrawText", "X": 40, "Y": 75, "Text": "datasource:weather_aveiro:codeText", "Font": 2, "Color": "White"},
  {"Type": "DrawText", "X": 10, "Y": 100, "Text": "datasource:weather_aveiro:temperature", "Prefix": "Exterior: ", "Suffix": " C", "Font": 2, "Color": "Yellow"}
]
```

Campos de `datasource:<Id>:<campo>`: `temperature`, `humidity`, `pressure`,
`wind`, `code`, `codeText` e `code_image`. Também existem os aliases `code_text`,
`codeImage` e `image`. Usar IDs curtos: a referência completa do objeto tem de
caber no respetivo campo de texto/imagem.

## HA — Home Assistant

`HA_URL` e `HA_TOKEN` vêm de `secrets.h`, com possíveis overrides de
`/secrets.json` no SD. As fontes HA usam esta configuração comum. Cada entidade
é consultada individualmente em `/api/states/<entity_id>`.

`Entities` aceita um array ou uma string separada por vírgulas; recomenda-se o
array. O grupo `Id` organiza as consultas. O desenho usa o **entity_id completo**,
sem o Id do grupo. `entity:<entity_id>` devolve o estado textual, não atributos.

### Portas — fonte doors

```json
[
  {
    "Type": "HA",
    "Id": "doors",
    "Refresh": 5000,
    "Entities": [
      "binary_sensor.porta_entrada",
      "binary_sensor.porta_cozinha",
      "binary_sensor.porta_alcapao",
      "binary_sensor.porta_centro",
      "binary_sensor.porta_cozinha_cave",
      "binary_sensor.porta_oficina"
    ]
  }
]
```

Exemplo de uma porta no ecrã; repetir o objeto com as outras entidades e posições:

```json
[
  {
    "Type": "DrawHASensor",
    "IconX": 198,
    "IconY": 35,
    "X": 228,
    "Y": 35,
    "Text": "Entrada",
    "Font": 2,
    "Color": "White",
    "entity": "binary_sensor.porta_entrada",
    "state:on": "DrawImage:door_open",
    "state:off": "DrawImage:door_closed",
    "else": "DrawImage:warning"
  }
]
```

`IconX/IconY` posicionam o ícone; `X/Y` posicionam a legenda. As regras
`state:<valor>` comparam o estado exato recebido. Se a integração devolver
`open`/`closed`, adicionar essas regras. `else` mostra o aviso quando não há
correspondência, incluindo estados desconhecidos. Existem até seis regras
por objeto; uma delas pode ser `else`.

### Presença — fonte people

```json
[
  {
    "Type": "HA",
    "Id": "people",
    "Refresh": 5000,
    "Entities": [
      "binary_sensor.pessoas_escada",
      "binary_sensor.pessoas_cozinha_cave",
      "binary_sensor.pessoas_na_salinha",
      "binary_sensor.pessoas_na_oficina"
    ]
  }
]
```

```json
[
  {
    "Type": "DrawHASensor",
    "IconX": 10,
    "IconY": 180,
    "X": 40,
    "Y": 180,
    "Text": "Escada",
    "Font": 2,
    "Color": "White",
    "entity": "binary_sensor.pessoas_escada",
    "state:on": "DrawImage:person_on",
    "state:off": "DrawImage:person_off",
    "else": "DrawImage:warning"
  }
]
```

A fonte `people` está declarada no layout atual, mas é preciso acrescentar
objetos como este para a tornar visível. Consultar estados no dashboard não
ativa os alertas Telegram do projeto Python da Raspi.

### Sala — temperatura, humidade e luz

Para voltar a usar estes sensores, inserir esta fonte em `DataSources`:

```json
[
  {
    "Type": "HA",
    "Id": "sala",
    "Refresh": 5000,
    "Entities": [
      "sensor.pessoas_sala_temperature",
      "sensor.pessoas_sala_humidity",
      "sensor.pessoas_sala_illuminance"
    ]
  }
]
```

E estes objetos em `Screens[].Objects`:

```json
[
  {"Type": "DrawText", "X": 10, "Y": 100, "Text": "entity:sensor.pessoas_sala_temperature", "Prefix": "Temp: ", "Suffix": " C", "Font": 4, "Color": "Yellow"},
  {"Type": "DrawText", "X": 10, "Y": 130, "Text": "entity:sensor.pessoas_sala_humidity", "Prefix": "Humidade: ", "Suffix": " %", "Font": 2, "Color": "Green"},
  {"Type": "DrawText", "X": 10, "Y": 155, "Text": "entity:sensor.pessoas_sala_illuminance", "Prefix": "Luz: ", "Suffix": " lux", "Font": 2, "Color": "Cyan"}
]
```

Estas posições são as mesmas que os pings ocupam agora: mover os pings ou
substituir os respetivos objetos ao recuperar a sala. `Prefix` é a grafia
recomendada; a grafia histórica `Preffix` também é aceite.

Os valores são apresentados como vêm do HA, sem arredondamento nem leitura
automática da unidade. `Prefix`/`Suffix` compõem o texto. Estados vazios,
`unknown` e `unavailable` aparecem como `--`, mantendo prefixo e sufixo.
Se um pedido HTTP falhar, o cache HA pode manter o último estado recebido;
o texto não é um indicador de atualidade da leitura.

## Ping — disponibilidade dos servidores

### Definir as fontes

```json
[
  {"Type": "Ping", "Id": "server1", "Host": "192.168.1.204", "Refresh": 10000},
  {"Type": "Ping", "Id": "server2", "Host": "192.168.1.75", "Refresh": 10000},
  {"Type": "Ping", "Id": "server3", "Host": "192.168.1.79", "Refresh": 10000}
]
```

`Host` aceita IPv4 ou hostname, por exemplo `raspi2` ou `raspi2.local`.
São enviados dois pedidos ICMP IPv4, com timeout de 2 segundos por pedido;
basta uma resposta para dar OK. Um hostname que falhe é tentado também com
`.local` por mDNS; IPs não recebem esse sufixo.

O intervalo é contado desde o fim da última verificação de cada servidor.
O default é 10000 ms e o valor é limitado a 1000..3600000 ms. As verificações
são sequenciais numa tarefa própria; servidores lentos podem atrasar a ronda,
sem manter bloqueado o mutex de desenho durante os pedidos.

### Mostrar texto na board

```json
[
  {"Type": "DrawText", "X": 10, "Y": 100, "Text": "ping:server1", "Prefix": "192.168.1.204 ", "Font": 2, "Color": "White"},
  {"Type": "DrawText", "X": 10, "Y": 130, "Text": "ping:server2", "Prefix": "192.168.1.75 ", "Font": 2, "Color": "White"},
  {"Type": "DrawText", "X": 10, "Y": 155, "Text": "ping:server3", "Prefix": "192.168.1.79 ", "Font": 2, "Color": "White"}
]
```

| Estado | Texto e cor |
| --- | --- |
| Ainda não verificado ou Id inexistente | `--`, branco |
| Respondeu | `OK`, verde |
| Sem resposta, falha de resolução/sessão ou Wi-Fi desligado após a primeira leitura | `FAIL`, amarelo |

Nos textos `ping:`, o firmware escolhe a cor de toda a linha automaticamente,
incluindo o prefixo, sobrepondo o campo `Color`.

### Mostrar um ícone booleano

```json
[
  {"Type": "DrawImageBool", "X": 10, "Y": 100, "Value": "ping:server1", "true": "DrawImage:wifi_on", "false": "DrawImage:wifi_off"}
]
```

Aqui os ícones Wi-Fi são apenas uma opção visual para representar o servidor.
`true` significa OK; FAIL, ainda não verificado e Id inexistente dão `false`.
Este objeto é uma alternativa ao texto na mesma posição.

## Funções locais — sem DataSource

Relógio, IP e estado do Wi-Fi não precisam de entradas em `DataSources`:

```json
[
  {"Type": "DrawText", "X": 5, "Y": 5, "Text": "function:Clock", "Font": 4, "Color": "White"},
  {"Type": "DrawText", "X": 160, "Y": 12, "Text": "function:MyIP", "Font": 1, "Color": "White"},
  {"Type": "DrawImageBool", "X": 290, "Y": 5, "Value": "function:IsWIFIConnected", "true": "DrawImage:wifi_on", "false": "DrawImage:wifi_off"}
]
```

Outros textos disponíveis: `function:Date`, `function:Uptime`,
`function:WifiRSSI` e `function:FreeRAM`. `function:IsHAConnected` também
existe no resolver booleano, mas indica apenas que há uma fonte HA marcada
válida no cache; não é uma prova de conectividade atual. MQTT não está
implementado.

## Logs, limites e aplicação do JSON

Para diagnosticar consultas, usar este objeto na raiz:

```json
{"Log": {"Weather": true, "HA": true, "Ping": true}}
```

As flags Weather/HA controlam também os logs HTTP dessas fontes. Ping regista
a primeira leitura e mudanças de estado. No uso normal podem ficar a `false`.

Limites atuais do firmware:

| Elemento | Limite |
| --- | --- |
| Fontes Weather/HA em conjunto | 16 |
| Fontes Ping | 8 |
| Entidades HA consultadas por grupo | 12 |
| Lista de entidades por grupo | 511 bytes, incluindo vírgulas |
| Entidades HA distintas no cache global | 40 |
| Id de fonte | Até 31 bytes; usar IDs únicos |
| Host de ping | Até 63 bytes |
| Mapeamentos meteorológicos Codes | 16 |
| Objetos por ecrã | 30 |
| Text/Image/expressão booleana de um objeto | Até 39 bytes |
| Prefix / Suffix | Até 23 / 15 bytes |

Os limites de texto são em bytes UTF-8. Referências HA longas também têm de
caber no campo `Text`; os exemplos da sala cabem. O estado HA em cache tem
até 15 bytes, pelo que estados textuais longos podem ser truncados.

Guardar o JSON no SD ou na origem configurada em `JSON_URL`, e escolher o nome
em `JSON_FILE`. Os overrides do SD em `/secrets.json` podem mudar essa seleção.
O ficheiro no SD tem prioridade sobre a rede. Depois usar
`http://<ip-da-board>/?reload=true` para reler o layout e as fontes.
Alterações só a este JSON não exigem recompilar o firmware, desde que a board
já tenha o dash001.4 para suportar Ping.

Referências de implementação: `datasources.h`, `datacache.h`, `functions.h`,
`ping_sources.h`, `objects.h` e `draw.h`.
