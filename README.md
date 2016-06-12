#Promptr

Wir haben ein AddIn für PowerPoint programmiert, dessen Aufgabe die Hilfestellung bei der zeitliche Einhaltung der Dauer einer Präsentation ist.

![Screenshot des AddIns in PowerPoint](https://github.com/famalgosner/philipshue/blob/master/Media/Screenshot.PNG)

*Screenshot des AddIns*

![Foto der Philips Hues im Präsentationsmodus](https://github.com/famalgosner/philipshue/blob/master/Media/PhotoHueBox.jpg)

*Foto der Philips Hues im Präsentationsmodus*

![Foto der Philips Hues im Präsentationsmodus](https://github.com/famalgosner/philipshue/blob/master/Media/ScreenshotApp.png)

*Screenshot der Feedback-App*

Dieses wird einfach und komfortabel installiert auf einem PC oder Mac über einen bereitgestellten Link.

##Hardware - Philips Hue

Das Gerät interagiert mit einer Station von 3 Philips Hue Lampen, welche mit Hilfe deren Bridge per WiFi angesprochen und verbunden wird.

##Ablauf

Der Countdown für den Farbwechsel der Lampen startet gemeinsam mit dem Start der PowerPoint Präsentation.

Jede Lampe steht für 1/(n=Anzahl Glühbirnen) der jeweiligen Zeitdauer der Präsentation, in unserem Fall also mit insgesamt 3 Minuten Präsentationsdauer steht somit jede Lampe für eine Minute und durchläuft einen Farbwechsel von einem hellen grün (Ein sattes Grün ist technisch nicht möglich (Gamut B)) hin zu einem dunklen rot und schaltet sich nach erreichen des dunkelsten Rottons ganz ab.

Die letzte Lampe (die dritte) gibt ab der Hälfte der ihr zugewiesenen Zeit (also zu Beginn der letzten 30 Sekunden zum Zeitpunkt 2min30s) ein extra Feedback, welches durch ein kurzes Pulsieren dargestellt wird, das gleiche passiert erneut nach erreichen der letzten 20 Sekunden mit einem kurzen aber etwas intensiveren Pulsieren und beim erreichen der letzten 10 Sekunden erfolgt das pulsieren im Sekundentakt bis zum Ablauf der Gesamtzeit.

Das Licht der Lampen erlischt komplett beim Ablauf der Gesamtzeit (3min).

##Weiteres

Ein weiteres tolles Feature ist das Feedback zu der Sprachgeschwindigkeit während der Präsentation. Diese wird vom Mikrofon des Notebooks aufgenommen und sorgt dafür, dass man in gleichbleibendem Tempo durch die Präsentation referiert.

Dies ist auf zwei Wegen möglich und zwar indem man die maximale Zeit für die gesamte Folie angibt und diese dann durch die Anzahl der Folien dividiert wird, oder, dass man jeder Folie seine Zeitdauer vorgibt und sich dadurch die Gesamtdauer der Präsentation ergibt.

Es erfolgt dann ein Hinweis für zu schnelles Vortragen in Form eines langsamen pulsieren, für zu langsames Vortragen pulsieren die Lampen als Hinweis schneller.

##Installation PowerPoint AddIn

Zur Installation des AddIns wird das vorkompilierte und gepackte AddIn im Ordner *Installation* in PowerPoint installiert.

*Momentan muss die IP und der Hue AppKey noch in der Constants-Klasse angegeben werden. In neueren Versionen wird dies allerdings über ein Einstellungsfeld definierbar sein.*

##Installation der Feedback App

Zur Installation der Feedback App kann die sich im Ordner *Installation* befindende apk-Datei auf dem Android-Smartphone installiert werden.

##Future Features

* Weitere Einstellungsmöglichkeiten für das AddIn (IP/AutoDiscover, AppKey, Helligkeit, ...)
* Automatische Erkennung der Sprachgeschwindigkeit
* ... 
