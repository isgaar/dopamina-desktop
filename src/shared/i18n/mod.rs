// Location: src/shared/i18n/mod.rs
// Function: Internationalization manager loading Android-style XML string
// catalogs (es, en) with automatic host locale detection and strict parity.
// All Rights Reserved isgaar

use std::collections::HashMap;

const STRINGS_ES: &str = include_str!("strings_es.xml");
const STRINGS_EN: &str = include_str!("strings_en.xml");

#[derive(Debug, Clone)]
pub struct StringCatalog {
    pub locale: String,
    es: HashMap<String, String>,
    en: HashMap<String, String>,
}

impl Default for StringCatalog {
    fn default() -> Self {
        Self::new("system")
    }
}

impl StringCatalog {
    pub fn new(language: &str) -> Self {
        let es = parse_strings_xml(STRINGS_ES);
        let en = parse_strings_xml(STRINGS_EN);
        let mut catalog = Self {
            locale: String::from("es"),
            es,
            en,
        };
        catalog.set_language(language);
        catalog
    }

    pub fn set_language(&mut self, language: &str) {
        let normalized = language.trim().to_lowercase();
        self.locale = match normalized.as_str() {
            "en" => String::from("en"),
            "es" => String::from("es"),
            _ => detect_host_locale(),
        };
    }

    pub fn is_spanish(&self) -> bool {
        self.locale == "es"
    }

    pub fn get<'a>(&'a self, key: &'a str) -> &'a str {
        let active = if self.locale == "en" {
            &self.en
        } else {
            &self.es
        };
        active
            .get(key)
            .or_else(|| self.es.get(key))
            .map(String::as_str)
            .unwrap_or(key)
    }

    pub fn get_or<'a>(&'a self, key: &'a str, fallback: &'a str) -> &'a str {
        let active = if self.locale == "en" {
            &self.en
        } else {
            &self.es
        };
        active
            .get(key)
            .or_else(|| self.es.get(key))
            .map(String::as_str)
            .unwrap_or(fallback)
    }
}

pub fn detect_host_locale() -> String {
    for var in ["LC_ALL", "LC_MESSAGES", "LANG"] {
        if let Ok(val) = std::env::var(var) {
            let lower = val.trim().to_lowercase();
            if lower.starts_with("en") {
                return String::from("en");
            }
            if lower.starts_with("es") {
                return String::from("es");
            }
        }
    }
    String::from("es")
}

fn parse_strings_xml(xml: &str) -> HashMap<String, String> {
    let mut map = HashMap::new();
    for line in xml.lines() {
        let line = line.trim();
        let Some(name_start) = line.find("name=\"") else {
            continue;
        };
        let after_name = &line[name_start + 6..];
        let Some(name_end) = after_name.find('"') else {
            continue;
        };
        let key = &after_name[..name_end];
        let Some(val_start) = after_name.find('>') else {
            continue;
        };
        let after_val = &after_name[val_start + 1..];
        let Some(val_end) = after_val.find("</string>") else {
            continue;
        };
        let val = unescape_xml(&after_val[..val_end]);
        map.insert(key.to_string(), val);
    }
    map
}

fn unescape_xml(raw: &str) -> String {
    raw.replace("&lt;", "<")
        .replace("&gt;", ">")
        .replace("&quot;", "\"")
        .replace("&apos;", "'")
        .replace("\\'", "'")
        .replace("&amp;", "&")
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn loads_es_and_en_catalogs_with_parity() {
        let cat = StringCatalog::new("es");
        assert!(!cat.es.is_empty());
        assert_eq!(cat.es.len(), cat.en.len());
        for key in cat.es.keys() {
            assert!(
                cat.en.contains_key(key),
                "Missing key in strings_en.xml: {key}"
            );
        }
    }
}

