/* ISO 3166 country/territory names from tzdata iso3166.tab (public domain),
   plus Kosovo (XK): 250 stable-key suggestions, not a sovereignty assertion. */
let items: array(Tea_ui.Combobox.item) = [|
  {
    key: "AD",
    label: {js|Andorra|js},
    disabled: false,
  },
  {
    key: "AE",
    label: {js|United Arab Emirates|js},
    disabled: false,
  },
  {
    key: "AF",
    label: {js|Afghanistan|js},
    disabled: false,
  },
  {
    key: "AG",
    label: {js|Antigua & Barbuda|js},
    disabled: false,
  },
  {
    key: "AI",
    label: {js|Anguilla|js},
    disabled: false,
  },
  {
    key: "AL",
    label: {js|Albania|js},
    disabled: false,
  },
  {
    key: "AM",
    label: {js|Armenia|js},
    disabled: false,
  },
  {
    key: "AO",
    label: {js|Angola|js},
    disabled: false,
  },
  {
    key: "AQ",
    label: {js|Antarctica|js},
    disabled: false,
  },
  {
    key: "AR",
    label: {js|Argentina|js},
    disabled: false,
  },
  {
    key: "AS",
    label: {js|Samoa (American)|js},
    disabled: false,
  },
  {
    key: "AT",
    label: {js|Austria|js},
    disabled: false,
  },
  {
    key: "AU",
    label: {js|Australia|js},
    disabled: false,
  },
  {
    key: "AW",
    label: {js|Aruba|js},
    disabled: false,
  },
  {
    key: "AX",
    label: {js|Åland Islands|js},
    disabled: false,
  },
  {
    key: "AZ",
    label: {js|Azerbaijan|js},
    disabled: false,
  },
  {
    key: "BA",
    label: {js|Bosnia & Herzegovina|js},
    disabled: false,
  },
  {
    key: "BB",
    label: {js|Barbados|js},
    disabled: false,
  },
  {
    key: "BD",
    label: {js|Bangladesh|js},
    disabled: false,
  },
  {
    key: "BE",
    label: {js|Belgium|js},
    disabled: false,
  },
  {
    key: "BF",
    label: {js|Burkina Faso|js},
    disabled: false,
  },
  {
    key: "BG",
    label: {js|Bulgaria|js},
    disabled: false,
  },
  {
    key: "BH",
    label: {js|Bahrain|js},
    disabled: false,
  },
  {
    key: "BI",
    label: {js|Burundi|js},
    disabled: false,
  },
  {
    key: "BJ",
    label: {js|Benin|js},
    disabled: false,
  },
  {
    key: "BL",
    label: {js|St Barthelemy|js},
    disabled: false,
  },
  {
    key: "BM",
    label: {js|Bermuda|js},
    disabled: false,
  },
  {
    key: "BN",
    label: {js|Brunei|js},
    disabled: false,
  },
  {
    key: "BO",
    label: {js|Bolivia|js},
    disabled: false,
  },
  {
    key: "BQ",
    label: {js|Caribbean NL|js},
    disabled: false,
  },
  {
    key: "BR",
    label: {js|Brazil|js},
    disabled: false,
  },
  {
    key: "BS",
    label: {js|Bahamas|js},
    disabled: false,
  },
  {
    key: "BT",
    label: {js|Bhutan|js},
    disabled: false,
  },
  {
    key: "BV",
    label: {js|Bouvet Island|js},
    disabled: false,
  },
  {
    key: "BW",
    label: {js|Botswana|js},
    disabled: false,
  },
  {
    key: "BY",
    label: {js|Belarus|js},
    disabled: false,
  },
  {
    key: "BZ",
    label: {js|Belize|js},
    disabled: false,
  },
  {
    key: "CA",
    label: {js|Canada|js},
    disabled: false,
  },
  {
    key: "CC",
    label: {js|Cocos (Keeling) Islands|js},
    disabled: false,
  },
  {
    key: "CD",
    label: {js|Congo (Dem. Rep.)|js},
    disabled: false,
  },
  {
    key: "CF",
    label: {js|Central African Rep.|js},
    disabled: false,
  },
  {
    key: "CG",
    label: {js|Congo (Rep.)|js},
    disabled: false,
  },
  {
    key: "CH",
    label: {js|Switzerland|js},
    disabled: false,
  },
  {
    key: "CI",
    label: {js|Côte d’Ivoire|js},
    disabled: false,
  },
  {
    key: "CK",
    label: {js|Cook Islands|js},
    disabled: false,
  },
  {
    key: "CL",
    label: {js|Chile|js},
    disabled: false,
  },
  {
    key: "CM",
    label: {js|Cameroon|js},
    disabled: false,
  },
  {
    key: "CN",
    label: {js|China|js},
    disabled: false,
  },
  {
    key: "CO",
    label: {js|Colombia|js},
    disabled: false,
  },
  {
    key: "CR",
    label: {js|Costa Rica|js},
    disabled: false,
  },
  {
    key: "CU",
    label: {js|Cuba|js},
    disabled: false,
  },
  {
    key: "CV",
    label: {js|Cape Verde|js},
    disabled: false,
  },
  {
    key: "CW",
    label: {js|Curaçao|js},
    disabled: false,
  },
  {
    key: "CX",
    label: {js|Christmas Island|js},
    disabled: false,
  },
  {
    key: "CY",
    label: {js|Cyprus|js},
    disabled: false,
  },
  {
    key: "CZ",
    label: {js|Czech Republic|js},
    disabled: false,
  },
  {
    key: "DE",
    label: {js|Germany|js},
    disabled: false,
  },
  {
    key: "DJ",
    label: {js|Djibouti|js},
    disabled: false,
  },
  {
    key: "DK",
    label: {js|Denmark|js},
    disabled: false,
  },
  {
    key: "DM",
    label: {js|Dominica|js},
    disabled: false,
  },
  {
    key: "DO",
    label: {js|Dominican Republic|js},
    disabled: false,
  },
  {
    key: "DZ",
    label: {js|Algeria|js},
    disabled: false,
  },
  {
    key: "EC",
    label: {js|Ecuador|js},
    disabled: false,
  },
  {
    key: "EE",
    label: {js|Estonia|js},
    disabled: false,
  },
  {
    key: "EG",
    label: {js|Egypt|js},
    disabled: false,
  },
  {
    key: "EH",
    label: {js|Western Sahara|js},
    disabled: false,
  },
  {
    key: "ER",
    label: {js|Eritrea|js},
    disabled: false,
  },
  {
    key: "ES",
    label: {js|Spain|js},
    disabled: false,
  },
  {
    key: "ET",
    label: {js|Ethiopia|js},
    disabled: false,
  },
  {
    key: "FI",
    label: {js|Finland|js},
    disabled: false,
  },
  {
    key: "FJ",
    label: {js|Fiji|js},
    disabled: false,
  },
  {
    key: "FK",
    label: {js|Falkland Islands|js},
    disabled: false,
  },
  {
    key: "FM",
    label: {js|Micronesia|js},
    disabled: false,
  },
  {
    key: "FO",
    label: {js|Faroe Islands|js},
    disabled: false,
  },
  {
    key: "FR",
    label: {js|France|js},
    disabled: false,
  },
  {
    key: "GA",
    label: {js|Gabon|js},
    disabled: false,
  },
  {
    key: "GB",
    label: {js|Britain (UK)|js},
    disabled: false,
  },
  {
    key: "GD",
    label: {js|Grenada|js},
    disabled: false,
  },
  {
    key: "GE",
    label: {js|Georgia|js},
    disabled: false,
  },
  {
    key: "GF",
    label: {js|French Guiana|js},
    disabled: false,
  },
  {
    key: "GG",
    label: {js|Guernsey|js},
    disabled: false,
  },
  {
    key: "GH",
    label: {js|Ghana|js},
    disabled: false,
  },
  {
    key: "GI",
    label: {js|Gibraltar|js},
    disabled: false,
  },
  {
    key: "GL",
    label: {js|Greenland|js},
    disabled: false,
  },
  {
    key: "GM",
    label: {js|Gambia|js},
    disabled: false,
  },
  {
    key: "GN",
    label: {js|Guinea|js},
    disabled: false,
  },
  {
    key: "GP",
    label: {js|Guadeloupe|js},
    disabled: false,
  },
  {
    key: "GQ",
    label: {js|Equatorial Guinea|js},
    disabled: false,
  },
  {
    key: "GR",
    label: {js|Greece|js},
    disabled: false,
  },
  {
    key: "GS",
    label: {js|South Georgia & the South Sandwich Islands|js},
    disabled: false,
  },
  {
    key: "GT",
    label: {js|Guatemala|js},
    disabled: false,
  },
  {
    key: "GU",
    label: {js|Guam|js},
    disabled: false,
  },
  {
    key: "GW",
    label: {js|Guinea-Bissau|js},
    disabled: false,
  },
  {
    key: "GY",
    label: {js|Guyana|js},
    disabled: false,
  },
  {
    key: "HK",
    label: {js|Hong Kong|js},
    disabled: false,
  },
  {
    key: "HM",
    label: {js|Heard Island & McDonald Islands|js},
    disabled: false,
  },
  {
    key: "HN",
    label: {js|Honduras|js},
    disabled: false,
  },
  {
    key: "HR",
    label: {js|Croatia|js},
    disabled: false,
  },
  {
    key: "HT",
    label: {js|Haiti|js},
    disabled: false,
  },
  {
    key: "HU",
    label: {js|Hungary|js},
    disabled: false,
  },
  {
    key: "ID",
    label: {js|Indonesia|js},
    disabled: false,
  },
  {
    key: "IE",
    label: {js|Ireland|js},
    disabled: false,
  },
  {
    key: "IL",
    label: {js|Israel|js},
    disabled: false,
  },
  {
    key: "IM",
    label: {js|Isle of Man|js},
    disabled: false,
  },
  {
    key: "IN",
    label: {js|India|js},
    disabled: false,
  },
  {
    key: "IO",
    label: {js|British Indian Ocean Territory|js},
    disabled: false,
  },
  {
    key: "IQ",
    label: {js|Iraq|js},
    disabled: false,
  },
  {
    key: "IR",
    label: {js|Iran|js},
    disabled: false,
  },
  {
    key: "IS",
    label: {js|Iceland|js},
    disabled: false,
  },
  {
    key: "IT",
    label: {js|Italy|js},
    disabled: false,
  },
  {
    key: "JE",
    label: {js|Jersey|js},
    disabled: false,
  },
  {
    key: "JM",
    label: {js|Jamaica|js},
    disabled: false,
  },
  {
    key: "JO",
    label: {js|Jordan|js},
    disabled: false,
  },
  {
    key: "JP",
    label: {js|Japan|js},
    disabled: false,
  },
  {
    key: "KE",
    label: {js|Kenya|js},
    disabled: false,
  },
  {
    key: "KG",
    label: {js|Kyrgyzstan|js},
    disabled: false,
  },
  {
    key: "KH",
    label: {js|Cambodia|js},
    disabled: false,
  },
  {
    key: "KI",
    label: {js|Kiribati|js},
    disabled: false,
  },
  {
    key: "KM",
    label: {js|Comoros|js},
    disabled: false,
  },
  {
    key: "KN",
    label: {js|St Kitts & Nevis|js},
    disabled: false,
  },
  {
    key: "KP",
    label: {js|Korea (North)|js},
    disabled: false,
  },
  {
    key: "KR",
    label: {js|Korea (South)|js},
    disabled: false,
  },
  {
    key: "KW",
    label: {js|Kuwait|js},
    disabled: false,
  },
  {
    key: "KY",
    label: {js|Cayman Islands|js},
    disabled: false,
  },
  {
    key: "KZ",
    label: {js|Kazakhstan|js},
    disabled: false,
  },
  {
    key: "LA",
    label: {js|Laos|js},
    disabled: false,
  },
  {
    key: "LB",
    label: {js|Lebanon|js},
    disabled: false,
  },
  {
    key: "LC",
    label: {js|St Lucia|js},
    disabled: false,
  },
  {
    key: "LI",
    label: {js|Liechtenstein|js},
    disabled: false,
  },
  {
    key: "LK",
    label: {js|Sri Lanka|js},
    disabled: false,
  },
  {
    key: "LR",
    label: {js|Liberia|js},
    disabled: false,
  },
  {
    key: "LS",
    label: {js|Lesotho|js},
    disabled: false,
  },
  {
    key: "LT",
    label: {js|Lithuania|js},
    disabled: false,
  },
  {
    key: "LU",
    label: {js|Luxembourg|js},
    disabled: false,
  },
  {
    key: "LV",
    label: {js|Latvia|js},
    disabled: false,
  },
  {
    key: "LY",
    label: {js|Libya|js},
    disabled: false,
  },
  {
    key: "MA",
    label: {js|Morocco|js},
    disabled: false,
  },
  {
    key: "MC",
    label: {js|Monaco|js},
    disabled: false,
  },
  {
    key: "MD",
    label: {js|Moldova|js},
    disabled: false,
  },
  {
    key: "ME",
    label: {js|Montenegro|js},
    disabled: false,
  },
  {
    key: "MF",
    label: {js|St Martin (French)|js},
    disabled: false,
  },
  {
    key: "MG",
    label: {js|Madagascar|js},
    disabled: false,
  },
  {
    key: "MH",
    label: {js|Marshall Islands|js},
    disabled: false,
  },
  {
    key: "MK",
    label: {js|North Macedonia|js},
    disabled: false,
  },
  {
    key: "ML",
    label: {js|Mali|js},
    disabled: false,
  },
  {
    key: "MM",
    label: {js|Myanmar (Burma)|js},
    disabled: false,
  },
  {
    key: "MN",
    label: {js|Mongolia|js},
    disabled: false,
  },
  {
    key: "MO",
    label: {js|Macau|js},
    disabled: false,
  },
  {
    key: "MP",
    label: {js|Northern Mariana Islands|js},
    disabled: false,
  },
  {
    key: "MQ",
    label: {js|Martinique|js},
    disabled: false,
  },
  {
    key: "MR",
    label: {js|Mauritania|js},
    disabled: false,
  },
  {
    key: "MS",
    label: {js|Montserrat|js},
    disabled: false,
  },
  {
    key: "MT",
    label: {js|Malta|js},
    disabled: false,
  },
  {
    key: "MU",
    label: {js|Mauritius|js},
    disabled: false,
  },
  {
    key: "MV",
    label: {js|Maldives|js},
    disabled: false,
  },
  {
    key: "MW",
    label: {js|Malawi|js},
    disabled: false,
  },
  {
    key: "MX",
    label: {js|Mexico|js},
    disabled: false,
  },
  {
    key: "MY",
    label: {js|Malaysia|js},
    disabled: false,
  },
  {
    key: "MZ",
    label: {js|Mozambique|js},
    disabled: false,
  },
  {
    key: "NA",
    label: {js|Namibia|js},
    disabled: false,
  },
  {
    key: "NC",
    label: {js|New Caledonia|js},
    disabled: false,
  },
  {
    key: "NE",
    label: {js|Niger|js},
    disabled: false,
  },
  {
    key: "NF",
    label: {js|Norfolk Island|js},
    disabled: false,
  },
  {
    key: "NG",
    label: {js|Nigeria|js},
    disabled: false,
  },
  {
    key: "NI",
    label: {js|Nicaragua|js},
    disabled: false,
  },
  {
    key: "NL",
    label: {js|Netherlands|js},
    disabled: false,
  },
  {
    key: "NO",
    label: {js|Norway|js},
    disabled: false,
  },
  {
    key: "NP",
    label: {js|Nepal|js},
    disabled: false,
  },
  {
    key: "NR",
    label: {js|Nauru|js},
    disabled: false,
  },
  {
    key: "NU",
    label: {js|Niue|js},
    disabled: false,
  },
  {
    key: "NZ",
    label: {js|New Zealand|js},
    disabled: false,
  },
  {
    key: "OM",
    label: {js|Oman|js},
    disabled: false,
  },
  {
    key: "PA",
    label: {js|Panama|js},
    disabled: false,
  },
  {
    key: "PE",
    label: {js|Peru|js},
    disabled: false,
  },
  {
    key: "PF",
    label: {js|French Polynesia|js},
    disabled: false,
  },
  {
    key: "PG",
    label: {js|Papua New Guinea|js},
    disabled: false,
  },
  {
    key: "PH",
    label: {js|Philippines|js},
    disabled: false,
  },
  {
    key: "PK",
    label: {js|Pakistan|js},
    disabled: false,
  },
  {
    key: "PL",
    label: {js|Poland|js},
    disabled: false,
  },
  {
    key: "PM",
    label: {js|St Pierre & Miquelon|js},
    disabled: false,
  },
  {
    key: "PN",
    label: {js|Pitcairn|js},
    disabled: false,
  },
  {
    key: "PR",
    label: {js|Puerto Rico|js},
    disabled: false,
  },
  {
    key: "PS",
    label: {js|Palestine|js},
    disabled: false,
  },
  {
    key: "PT",
    label: {js|Portugal|js},
    disabled: false,
  },
  {
    key: "PW",
    label: {js|Palau|js},
    disabled: false,
  },
  {
    key: "PY",
    label: {js|Paraguay|js},
    disabled: false,
  },
  {
    key: "QA",
    label: {js|Qatar|js},
    disabled: false,
  },
  {
    key: "RE",
    label: {js|Réunion|js},
    disabled: false,
  },
  {
    key: "RO",
    label: {js|Romania|js},
    disabled: false,
  },
  {
    key: "RS",
    label: {js|Serbia|js},
    disabled: false,
  },
  {
    key: "RU",
    label: {js|Russia|js},
    disabled: false,
  },
  {
    key: "RW",
    label: {js|Rwanda|js},
    disabled: false,
  },
  {
    key: "SA",
    label: {js|Saudi Arabia|js},
    disabled: false,
  },
  {
    key: "SB",
    label: {js|Solomon Islands|js},
    disabled: false,
  },
  {
    key: "SC",
    label: {js|Seychelles|js},
    disabled: false,
  },
  {
    key: "SD",
    label: {js|Sudan|js},
    disabled: false,
  },
  {
    key: "SE",
    label: {js|Sweden|js},
    disabled: false,
  },
  {
    key: "SG",
    label: {js|Singapore|js},
    disabled: false,
  },
  {
    key: "SH",
    label: {js|St Helena|js},
    disabled: false,
  },
  {
    key: "SI",
    label: {js|Slovenia|js},
    disabled: false,
  },
  {
    key: "SJ",
    label: {js|Svalbard & Jan Mayen|js},
    disabled: false,
  },
  {
    key: "SK",
    label: {js|Slovakia|js},
    disabled: false,
  },
  {
    key: "SL",
    label: {js|Sierra Leone|js},
    disabled: false,
  },
  {
    key: "SM",
    label: {js|San Marino|js},
    disabled: false,
  },
  {
    key: "SN",
    label: {js|Senegal|js},
    disabled: false,
  },
  {
    key: "SO",
    label: {js|Somalia|js},
    disabled: false,
  },
  {
    key: "SR",
    label: {js|Suriname|js},
    disabled: false,
  },
  {
    key: "SS",
    label: {js|South Sudan|js},
    disabled: false,
  },
  {
    key: "ST",
    label: {js|Sao Tome & Principe|js},
    disabled: false,
  },
  {
    key: "SV",
    label: {js|El Salvador|js},
    disabled: false,
  },
  {
    key: "SX",
    label: {js|St Maarten (Dutch)|js},
    disabled: false,
  },
  {
    key: "SY",
    label: {js|Syria|js},
    disabled: false,
  },
  {
    key: "SZ",
    label: {js|Eswatini (Swaziland)|js},
    disabled: false,
  },
  {
    key: "TC",
    label: {js|Turks & Caicos Is|js},
    disabled: false,
  },
  {
    key: "TD",
    label: {js|Chad|js},
    disabled: false,
  },
  {
    key: "TF",
    label: {js|French S. Terr.|js},
    disabled: false,
  },
  {
    key: "TG",
    label: {js|Togo|js},
    disabled: false,
  },
  {
    key: "TH",
    label: {js|Thailand|js},
    disabled: false,
  },
  {
    key: "TJ",
    label: {js|Tajikistan|js},
    disabled: false,
  },
  {
    key: "TK",
    label: {js|Tokelau|js},
    disabled: false,
  },
  {
    key: "TL",
    label: {js|East Timor|js},
    disabled: false,
  },
  {
    key: "TM",
    label: {js|Turkmenistan|js},
    disabled: false,
  },
  {
    key: "TN",
    label: {js|Tunisia|js},
    disabled: false,
  },
  {
    key: "TO",
    label: {js|Tonga|js},
    disabled: false,
  },
  {
    key: "TR",
    label: {js|Turkey|js},
    disabled: false,
  },
  {
    key: "TT",
    label: {js|Trinidad & Tobago|js},
    disabled: false,
  },
  {
    key: "TV",
    label: {js|Tuvalu|js},
    disabled: false,
  },
  {
    key: "TW",
    label: {js|Taiwan|js},
    disabled: false,
  },
  {
    key: "TZ",
    label: {js|Tanzania|js},
    disabled: false,
  },
  {
    key: "UA",
    label: {js|Ukraine|js},
    disabled: false,
  },
  {
    key: "UG",
    label: {js|Uganda|js},
    disabled: false,
  },
  {
    key: "UM",
    label: {js|US minor outlying islands|js},
    disabled: false,
  },
  {
    key: "US",
    label: {js|United States|js},
    disabled: false,
  },
  {
    key: "UY",
    label: {js|Uruguay|js},
    disabled: false,
  },
  {
    key: "UZ",
    label: {js|Uzbekistan|js},
    disabled: false,
  },
  {
    key: "VA",
    label: {js|Vatican City|js},
    disabled: false,
  },
  {
    key: "VC",
    label: {js|St Vincent|js},
    disabled: false,
  },
  {
    key: "VE",
    label: {js|Venezuela|js},
    disabled: false,
  },
  {
    key: "VG",
    label: {js|Virgin Islands (UK)|js},
    disabled: false,
  },
  {
    key: "VI",
    label: {js|Virgin Islands (US)|js},
    disabled: false,
  },
  {
    key: "VN",
    label: {js|Vietnam|js},
    disabled: false,
  },
  {
    key: "VU",
    label: {js|Vanuatu|js},
    disabled: false,
  },
  {
    key: "WF",
    label: {js|Wallis & Futuna|js},
    disabled: false,
  },
  {
    key: "WS",
    label: {js|Samoa (western)|js},
    disabled: false,
  },
  {
    key: "YE",
    label: {js|Yemen|js},
    disabled: false,
  },
  {
    key: "YT",
    label: {js|Mayotte|js},
    disabled: false,
  },
  {
    key: "ZA",
    label: {js|South Africa|js},
    disabled: false,
  },
  {
    key: "ZM",
    label: {js|Zambia|js},
    disabled: false,
  },
  {
    key: "ZW",
    label: {js|Zimbabwe|js},
    disabled: false,
  },
  {
    key: "XK",
    label: {js|Kosovo|js},
    disabled: false,
  },
|];
