voir https://www.rcgroups.com/forums/showpost.php?p=33359346&postcount=332

Bonjour Pawel,

merci pour tous les efforts que vous mettez dans cette grande biblioth�que.
J'appr�cie �galement beaucoup l'option de d�codage. 
Peut-�tre que je l'utilise d'une mani�re diff�rente de celle � laquelle il �tait destin�.
J'aime piloter des ailes fixes, mais j'ai aussi un bateau rc. 
Un bateau rc est une maison de poup�e pour gar�ons o� vous souhaitez ajouter beaucoup de fonctionnalit�s. 
Cela n�cessite un grand nombre de canaux de commutation. 
Il existe des solutions comme les modules Graupner Nautic sur le march�, mais avec votre biblioth�que, nous pouvons le faire mieux et moins cher.

Normalement, nous connectons un capteur au r�cepteur et envoyons des donn�es de t�l�m�trie au Taranis. 
Le jeu fonctionne aussi dans l'autre sens. 
Il est possible de connecter un capteur � la broche S.Port (broche en bas du connecteur pour le module TX externe) et les 
donn�es seront transf�r�es vers le connecteur S.Port du r�cepteur.

Pour utiliser cette fonctionnalit�, j'ai modifi� l'un des capteurs de votre biblioth�que et l'ai renomm� en transfertx. 
Il r�pond � ID20 et utilise les adresses 0x1900 et 0x1910. Cela me donne la possibilit� de transf�rer 64 bits r�partis en deux mots 
de 32 bits du Taranis au r�cepteur environ deux fois par seconde.
J'ai joint un fichier zip contenant un exemple et deux fichiers comme extension pour la biblioth�que. 
Pour essayer cela, copiez simplement FrSkySportSensorTransferTX.h et FrSkySportSensorTransferTX.cpp dans le dossier de la biblioth�que FrSkySportTelemetry. Ajoutez en option "FrSkySportSensorTransferTX KEYWORD1" � keywords.txt

Vous aurez besoin de deux Arduinos comme Uno, Nano 3.0, pro mini comme d'habitude. 
Mettez � jour l'un d'eux avec "FrSkySport_Transfertx_transmitter_v20151204" et connectez-le au Taranis l'autre va avec "FrSkySport_Transfertx_receiver_v20151204" au r�cepteur. Peset pour la broche S.Port est la broche 12. Vous pouvez connecter des commutateurs � la terre � la broche num�rique 0 � 11, � la broche num�rique 13 et � la broche analogique A0 � A5 du c�t� Taranis, puis vous obtiendrez une sortie 1 par 1 du c�t� du r�cepteur. pour toutes les 19 broches.

Ceci est juste un exemple simple pour montrer comment cela fonctionne. 
La prochaine �tape consiste � obtenir une meilleure interface utilisateur pour toutes les fonctions du bateau. 
La mani�re normale est de remplir l'�metteur avec des interrupteurs. Que diriez-vous de laisser les fonctions principales comme le 
gouvernail motoris� et ainsi de suite � l'�metteur et de mettre les fonctions suppl�mentaires sur un smartphone ou une tablette ? 
Je crois que c'est la voie la plus intelligente pour le 20�me si�cle. Cela donne �galement la possibilit� de partager le plaisir 
avec le bateau avec une deuxi�me personne comme un enfant.

Si quelqu'un est int�ress� par cela, je peux travailler sur un exemple.

