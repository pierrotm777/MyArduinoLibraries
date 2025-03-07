voir https://www.rcgroups.com/forums/showpost.php?p=33359346&postcount=332

Bonjour Pawel,

merci pour tous les efforts que vous mettez dans cette grande bibliothèque.
J'apprécie également beaucoup l'option de décodage. 
Peut-être que je l'utilise d'une manière différente de celle à laquelle il était destiné.
J'aime piloter des ailes fixes, mais j'ai aussi un bateau rc. 
Un bateau rc est une maison de poupée pour garçons où vous souhaitez ajouter beaucoup de fonctionnalités. 
Cela nécessite un grand nombre de canaux de commutation. 
Il existe des solutions comme les modules Graupner Nautic sur le marché, mais avec votre bibliothèque, nous pouvons le faire mieux et moins cher.

Normalement, nous connectons un capteur au récepteur et envoyons des données de télémétrie au Taranis. 
Le jeu fonctionne aussi dans l'autre sens. 
Il est possible de connecter un capteur à la broche S.Port (broche en bas du connecteur pour le module TX externe) et les 
données seront transférées vers le connecteur S.Port du récepteur.

Pour utiliser cette fonctionnalité, j'ai modifié l'un des capteurs de votre bibliothèque et l'ai renommé en transfertx. 
Il répond à ID20 et utilise les adresses 0x1900 et 0x1910. Cela me donne la possibilité de transférer 64 bits répartis en deux mots 
de 32 bits du Taranis au récepteur environ deux fois par seconde.
J'ai joint un fichier zip contenant un exemple et deux fichiers comme extension pour la bibliothèque. 
Pour essayer cela, copiez simplement FrSkySportSensorTransferTX.h et FrSkySportSensorTransferTX.cpp dans le dossier de la bibliothèque FrSkySportTelemetry. Ajoutez en option "FrSkySportSensorTransferTX KEYWORD1" à keywords.txt

Vous aurez besoin de deux Arduinos comme Uno, Nano 3.0, pro mini comme d'habitude. 
Mettez à jour l'un d'eux avec "FrSkySport_Transfertx_transmitter_v20151204" et connectez-le au Taranis l'autre va avec "FrSkySport_Transfertx_receiver_v20151204" au récepteur. Peset pour la broche S.Port est la broche 12. Vous pouvez connecter des commutateurs à la terre à la broche numérique 0 à 11, à la broche numérique 13 et à la broche analogique A0 à A5 du côté Taranis, puis vous obtiendrez une sortie 1 par 1 du côté du récepteur. pour toutes les 19 broches.

Ceci est juste un exemple simple pour montrer comment cela fonctionne. 
La prochaine étape consiste à obtenir une meilleure interface utilisateur pour toutes les fonctions du bateau. 
La manière normale est de remplir l'émetteur avec des interrupteurs. Que diriez-vous de laisser les fonctions principales comme le 
gouvernail motorisé et ainsi de suite à l'émetteur et de mettre les fonctions supplémentaires sur un smartphone ou une tablette ? 
Je crois que c'est la voie la plus intelligente pour le 20ème siècle. Cela donne également la possibilité de partager le plaisir 
avec le bateau avec une deuxième personne comme un enfant.

Si quelqu'un est intéressé par cela, je peux travailler sur un exemple.

