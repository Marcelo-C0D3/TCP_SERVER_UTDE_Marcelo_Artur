
# TCP Server UTDE(Umidade, Temperatura, Distancia, Estado)


O aplicativo cria um socket TCP com o número de porta especificado e aguarda uma solicitação de conexão do cliente. Depois de aceitar uma solicitação do cliente, a conexão entre o servidor e o cliente é estabelecida e o aplicativo aguarda que alguns dados sejam recebidos do cliente. Os dados recebidos são processados como texto ASCII e retorna a resposta ao cliente cuja oque foi solicitado.

## Como usar o Aplicativo


### Requisitos de Hardware

```
* Este exemplo pode ser executado em qualquer placa de desenvolvimento ESP32 comumente disponível.
* Necessario sensor DHT11(Defina os pinos no cabeçario do projeto/main.c)Default(GPIO_NUM_0).
* Necessario sensor HC-SR04(Defina os pinos no cabeçario do projeto/main.c)Default(GPIO_NUM_2/GPIO_NUM_13).
* Necessario LED(Defina os pinos no cabeçario do projeto/main.c) Default(`GPIO_NUM_14`).
```

### Configure o Projeto

* Navegue até a pasta do projeto, com seu console do SDK (ESP-IDF Command Prompt) .
```
idf.py menuconfig
```
#### Após rodar o comando a seguinte tela deve aparecer:

![menuconfig](https://user-images.githubusercontent.com/56330822/99081292-5884e980-25a1-11eb-9a85-5e683d7ec984.PNG)

#### Acesse `Example Configuration` .

![port](https://user-images.githubusercontent.com/56330822/99081795-190acd00-25a2-11eb-8f07-d66c372a836d.PNG)

#### Configure a Porta `Port`, se quiser. Por padrão está executando a porta 3333.
#### Retorne a tela anterior.

#### Acesse `Example Connection Configuration` .

![Redeconfig](https://user-images.githubusercontent.com/56330822/99082473-f4fbbb80-25a2-11eb-8547-e671b19ec0ef.PNG)

#### Configure o Nome de sua rede `WiFi SSID`. Necessario para executar o servidor.
#### Configure A Senha da sua rede `WiFi Password`. Necessario para executar o servidor.
#### Desmarque a caixa `Obtain IPv6 link-local address`. Vem selecionada por padrão, e desnecessario para uso em redes IPv4.
#### Apos os procedimentos. Salve as alteraçoes e Saia do menuconfig..


### Build and Flash

Ainda na pasta do projeto, com seu console do SDK (ESP-IDF Command Prompt) 
Construa o projeto e envie-o para a placa, em seguida, execute a ferramenta de monitoramento para visualizar a saída serial:

```
idf.py -p PORT flash monitor
```

#### Após rodar o comando a seguinte tela deve aparecer:

![rodando](https://user-images.githubusercontent.com/56330822/99083292-190bcc80-25a4-11eb-8d32-2093400483a2.PNG)

## Otimo! O servidor está no Ar.

#### Em vermelho na imagem, visualize o `IP` que o servidor obteve e em sequencia a `Porta` que definimos. Observe no seu console os dados atribuidos, e os guarde, que iremos utilizar em breve.

### Estabelecer uma conxão Cliente/Servidor.

Inicie primeiro o servidor para receber os dados solicitados pelo cliente (aplicativo).
