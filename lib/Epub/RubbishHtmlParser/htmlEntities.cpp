#include "htmlEntities.h"
#include <string>
#include <unordered_map>

const int MAX_ENTITY_LENGTH = 10;

// Use book: entities_ww2.epub to test this (Page 7: Entities parser test)
// Note the supported keys are only in lowercase
// Store the mappings in a unordered hash map
static std::unordered_map<std::string, std::string> entity_lookup(
    {{"&quot;", "\""},
     {"&frasl;", "⁄"},
     {"&amp;", "&"},
     {"&lt;", "<"},
     {"&gt;", ">"},
     {"&Agrave;", "À"},
     {"&Aacute;", "Á"},
     {"&Acirc;", "Â"},
     {"&Atilde;", "Ã"},
     {"&Auml;", "Ä"},
     {"&Aring;", "Å"},
     {"&AElig;", "Æ"},
     {"&Ccedil;", "Ç"},
     {"&Egrave;", "È"},
     {"&Eacute;", "É"},
     {"&Ecirc;", "Ê"},
     {"&Euml;", "Ë"},
     {"&Igrave;", "Ì"},
     {"&Iacute;", "Í"},
     {"&Icirc;", "Î"},
     {"&Iuml;", "Ï"},
     {"&ETH;", "Ð"},
     {"&Ntilde;", "Ñ"},
     {"&Ograve;", "Ò"},
     {"&Oacute;", "Ó"},
     {"&Ocirc;", "Ô"},
     {"&Otilde;", "Õ"},
     {"&Ouml;", "Ö"},
     {"&Oslash;", "Ø"},
     {"&Ugrave;", "Ù"},
     {"&Uacute;", "Ú"},
     {"&Ucirc;", "Û"},
     {"&Uuml;", "Ü"},
     {"&Yacute;", "Ý"},
     {"&THORN;", "Þ"},
     {"&szlig;", "ß"},
     {"&agrave;", "à"},
     {"&aacute;", "á"},
     {"&acirc;", "â"},
     {"&atilde;", "ã"},
     {"&auml;", "ä"},
     {"&aring;", "å"},
     {"&aelig;", "æ"},
     {"&ccedil;", "ç"},
     {"&egrave;", "è"},
     {"&eacute;", "é"},
     {"&ecirc;", "ê"},
     {"&euml;", "ë"},
     {"&igrave;", "ì"},
     {"&iacute;", "í"},
     {"&icirc;", "î"},
     {"&iuml;", "ï"},
     {"&eth;", "ð"},
     {"&ntilde;", "ñ"},
     {"&ograve;", "ò"},
     {"&oacute;", "ó"},
     {"&ocirc;", "ô"},
     {"&otilde;", "õ"},
     {"&ouml;", "ö"},
     {"&oslash;", "ø"},
     {"&ugrave;", "ù"},
     {"&uacute;", "ú"},
     {"&ucirc;", "û"},
     {"&uuml;", "ü"},
     {"&yacute;", "ý"},
     {"&thorn;", "þ"},
     {"&yuml;", "ÿ"},
     {"&nbsp;", ""},
     {"&iexcl;", "¡"},
     {"&cent;", "¢"},
     {"&pound;", "£"},
     {"&curren;", "¤"},
     {"&yen;", "¥"},
     {"&brvbar;", "¦"},
     {"&sect;", "§"},
     {"&uml;", "¨"},
     {"&copy;", "©"},
     {"&ordf;", "ª"},
     {"&laquo;", "«"},
     {"&not;", "¬"},
     {"&shy;", "­"},
     {"&reg;", "®"},
     {"&macr;", "¯"},
     {"&deg;", "°"},
     {"&plusmn;", "±"},
     {"&sup2;", "²"},
     {"&sup3;", "³"},
     {"&acute;", "´"},
     {"&micro;", "µ"},
     {"&para;", "¶"},
     {"&cedil;", "¸"},
     {"&sup1;", "¹"},
     {"&ordm;", "º"},
     {"&raquo;", "»"},
     {"&frac14;", "¼"},
     {"&frac12;", "½"},
     {"&frac34;", "¾"},
     {"&iquest;", "¿"},
     {"&times;", "×"},
     {"&divide;", "÷"},
     {"&forall;", "∀"},
     {"&part;", "∂"},
     {"&exist;", "∃"},
     {"&empty;", "∅"},
     {"&nabla;", "∇"},
     {"&isin;", "∈"},
     {"&notin;", "∉"},
     {"&ni;", "∋"},
     {"&prod;", "∏"},
     {"&sum;", "∑"},
     {"&minus;", "−"},
     {"&lowast;", "∗"},
     {"&radic;", "√"},
     {"&prop;", "∝"},
     {"&infin;", "∞"},
     {"&ang;", "∠"},
     {"&and;", "∧"},
     {"&or;", "∨"},
     {"&cap;", "∩"},
     {"&cup;", "∪"},
     {"&int;", "∫"},
     {"&there4;", "∴"},
     {"&sim;", "∼"},
     {"&cong;", "≅"},
     {"&asymp;", "≈"},
     {"&ne;", "≠"},
     {"&equiv;", "≡"},
     {"&le;", "≤"},
     {"&ge;", "≥"},
     {"&sub;", "⊂"},
     {"&sup;", "⊃"},
     {"&nsub;", "⊄"},
     {"&sube;", "⊆"},
     {"&supe;", "⊇"},
     {"&oplus;", "⊕"},
     {"&otimes;", "⊗"},
     {"&perp;", "⊥"},
     {"&sdot;", "⋅"},
     {"&Alpha;", "Α"},
     {"&Beta;", "Β"},
     {"&Gamma;", "Γ"},
     {"&Delta;", "Δ"},
     {"&Epsilon;", "Ε"},
     {"&Zeta;", "Ζ"},
     {"&Eta;", "Η"},
     {"&Theta;", "Θ"},
     {"&Iota;", "Ι"},
     {"&Kappa;", "Κ"},
     {"&Lambda;", "Λ"},
     {"&Mu;", "Μ"},
     {"&Nu;", "Ν"},
     {"&Xi;", "Ξ"},
     {"&Omicron;", "Ο"},
     {"&Pi;", "Π"},
     {"&Rho;", "Ρ"},
     {"&Sigma;", "Σ"},
     {"&Tau;", "Τ"},
     {"&Upsilon;", "Υ"},
     {"&Phi;", "Φ"},
     {"&Chi;", "Χ"},
     {"&Psi;", "Ψ"},
     {"&Omega;", "Ω"},
     {"&alpha;", "α"},
     {"&beta;", "β"},
     {"&gamma;", "γ"},
     {"&delta;", "δ"},
     {"&epsilon;", "ε"},
     {"&zeta;", "ζ"},
     {"&eta;", "η"},
     {"&theta;", "θ"},
     {"&iota;", "ι"},
     {"&kappa;", "κ"},
     {"&lambda;", "λ"},
     {"&mu;", "μ"},
     {"&nu;", "ν"},
     {"&xi;", "ξ"},
     {"&omicron;", "ο"},
     {"&pi;", "π"},
     {"&rho;", "ρ"},
     {"&sigmaf;", "ς"},
     {"&sigma;", "σ"},
     {"&tau;", "τ"},
     {"&upsilon;", "υ"},
     {"&phi;", "φ"},
     {"&chi;", "χ"},
     {"&psi;", "ψ"},
     {"&omega;", "ω"},
     {"&thetasym;", "ϑ"},
     {"&upsih;", "ϒ"},
     {"&piv;", "ϖ"},
     {"&OElig;", "Œ"},
     {"&oelig;", "œ"},
     {"&Scaron;", "Š"},
     {"&scaron;", "š"},
     {"&Yuml;", "Ÿ"},
     {"&fnof;", "ƒ"},
     {"&circ;", "ˆ"},
     {"&tilde;", "˜"},
     {"&ensp;", ""},
     {"&emsp;", ""},
     {"&thinsp;", ""},
     {"&zwnj;", "‌"},
     {"&zwj;", "‍"},
     {"&lrm;", "‎"},
     {"&rlm;", "‏"},
     {"&ndash;", "–"},
     {"&mdash;", "—"},
     {"&lsquo;", "‘"},
     {"&rsquo;", "’"},
     {"&sbquo;", "‚"},
     {"&ldquo;", "“"},
     {"&rdquo;", "”"},
     {"&bdquo;", "„"},
     {"&dagger;", "†"},
     {"&Dagger;", "‡"},
     {"&bull;", "•"},
     {"&hellip;", "…"},
     {"&permil;", "‰"},
     {"&prime;", "′"},
     {"&Prime;", "″"},
     {"&lsaquo;", "‹"},
     {"&rsaquo;", "›"},
     {"&oline;", "‾"},
     {"&euro;", "€"},
     {"&trade;", "™"},
     {"&larr;", "←"},
     {"&uarr;", "↑"},
     {"&rarr;", "→"},
     {"&darr;", "↓"},
     {"&harr;", "↔"},
     {"&crarr;", "↵"},
     {"&lceil;", "⌈"},
     {"&rceil;", "⌉"},
     {"&lfloor;", "⌊"},
     {"&rfloor;", "⌋"},
     {"&loz;", "◊"},
     {"&spades;", "♠"},
     {"&clubs;", "♣"},
     {"&hearts;", "♥"},
     {"&diams;", "♦"}});

void convert_to_utf8(int code, std::string &res)
{
  // convert to a utf8 sequence
  if (code < 0x80)
  {
    res += static_cast<char>(code);
  }
  else if (code < 0x800)
  {
    res += static_cast<char>(0xc0 | (code >> 6));
    res += static_cast<char>(0x80 | (code & 0x3f));
  }
  else if (code < 0x10000)
  {
    res += static_cast<char>(0xe0 | (code >> 12));
    res += static_cast<char>(0x80 | ((code >> 6) & 0x3f));
    res += static_cast<char>(0x80 | (code & 0x3f));
  }
  else if (code < 0x200000)
  {
    res += static_cast<char>(0xf0 | (code >> 18));
    res += static_cast<char>(0x80 | ((code >> 12) & 0x3f));
    res += static_cast<char>(0x80 | ((code >> 6) & 0x3f));
    res += static_cast<char>(0x80 | (code & 0x3f));
  }
  else if (code < 0x4000000)
  {
    res += static_cast<char>(0xf8 | (code >> 24));
    res += static_cast<char>(0x80 | ((code >> 18) & 0x3f));
    res += static_cast<char>(0x80 | ((code >> 12) & 0x3f));
    res += static_cast<char>(0x80 | ((code >> 6) & 0x3f));
    res += static_cast<char>(0x80 | (code & 0x3f));
  }
  else if (code < 0x80000000)
  {
    res += static_cast<char>(0xfc | (code >> 30));
    res += static_cast<char>(0x80 | ((code >> 24) & 0x3f));
    res += static_cast<char>(0x80 | ((code >> 18) & 0x3f));
    res += static_cast<char>(0x80 | ((code >> 12) & 0x3f));
    res += static_cast<char>(0x80 | ((code >> 6) & 0x3f));
  }
}

bool process_numeric_entity(const std::string &entity, std::string &res)
{
  int code = 0;
  // is it hex?
  if (entity[2] == 'x' || entity[2] == 'X')
  {
    // parse the hex code
    code = strtol(entity.substr(3, entity.size() - 3).c_str(), nullptr, 16);
  }
  else
  {
    code = strtol(entity.substr(2, entity.size() - 3).c_str(), nullptr, 10);
  }
  if (code != 0)
  {
    convert_to_utf8(code, res);
    return true;
  }
  return false;
}

bool process_string_entity(const std::string &entity, std::string &res)
{
  // it's a named entity - find it in the lookup table
  // find it in the map
  auto it = entity_lookup.find(entity);
  if (it != entity_lookup.end())
  {
    res += it->second;
    return true;
  }
  return false;
}

// TODO - handle numeric entities
std::string replace_html_entities(const std::string &text)
{
  std::string res;
  res.reserve(text.size());
  for (int i = 0; i < text.size(); ++i)
  {
    bool flag = false;
    // do we have a potential entity?
    if (text[i] == '&')
    {
      // find the end of the entity
      int j = i + 1;
      while (j < text.size() && text[j] != ';' && j - i < MAX_ENTITY_LENGTH)
      {
        j++;
      }
      if (j - i > 2)
      {
        std::string entity = text.substr(i, j - i + 1);
        // is it a numeric code?
        if (entity[1] == '#')
        {
          flag = process_numeric_entity(entity, res);
        }
        else
        {
          flag = process_string_entity(entity, res);
        }
        // skip past the entity if we successfully decoded it
        if (flag)
        {
          i = j;
        }
      }
    }
    if (!flag)
    {
      res += text[i];
    }
  }
  return res;
}
