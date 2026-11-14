# ESP32-1732S019 (C/N)

Etat: **hardware manifest / full port not ready**.

- LCD : ST7789 SPI, 170 x 320
- Tactile : GT911 (C), none (N)
- Avertissement : S019C et S019N ne sont pas interchangeables; le panneau 170x320 comporte un offset ST7789 et le portage complet necessite les broches confirmes du constructeur

Les versions et les GPIO des fabricants peuvent changer. Aucun resultat de compilation ni essai physique n est affirme dans cette version.

### Profil experimental ajoute dans V0.3.23

`dashboardesp_1732s019c_portrait_EXPERIMENTAL` utilise les GPIO publies :
LCD MOSI=13, CLK=12, CS=10, DC=11, RST=1, BL=14.
GT911 SDA=9, SCL=46, INT=8 (non utilise par interrogation), RST=3.
Le profil C est prevu pour essais et n'a pas ete compile ni valide sur le materiel. Les offsets ST7789 et les axes du tactile restent a confirmer. Le firmware complet tient compte de la resolution 170x320 pour l'interface, mais l'ergonomie de widgets complexes exige un design adapte.
