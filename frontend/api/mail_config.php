<?php
/**
 * ============================================================================
 * NETCUP MAIL-KONFIGURATION (Portfolio & Shop Kontaktformular)
 * ============================================================================
 * Anleitung für dein Netcup Webhosting-Paket:
 * 
 * 1. Lege im Netcup CustomerControlPanel (CCP) -> "E-Mail" ein Postfach an,
 *    z.B. "kontakt@michaelkrahn.de" (oder "noreply@michaelkrahn.de").
 * 
 * 2. MODUS A: Direkt über Netcup PHP mail() (Standard, funktioniert sofort auf Netcup)
 *    - Setze 'use_smtp' => false
 *    - 'from_email' MUSS eine existierende E-Mail-Adresse deiner Netcup-Domain sein!
 *    - 'recipient_email' kann deine Hauptadresse (z.B. michael.penner97@web.de oder
 *      kontakt@michaelkrahn.de) sein.
 * 
 * 3. MODUS B: Authentifizierter SMTP-Versand über deinen Netcup Mailserver (Empfohlen für 100% Zustellrate & DKIM)
 *    - Setze 'use_smtp' => true
 *    - Trage deinen Netcup-Mailserver (z.B. 'mail.michaelkrahn.de' oder den im CCP
 *      angezeigten Servernamen wie 'mx2f12.netcup.net') und das Passwort ein.
 */

return [
    // Ziel-Adresse, an die alle Portfolio- & Shop-Anfragen gesendet werden:
    'recipient_email' => 'info@michaelkrahn.de',
    'recipient_name'  => 'Michael Krahn – Creative Studio',

    // Absender-Adresse (Muss bei Netcup zu deiner gehosteten Domain gehören!):
    'from_email'      => 'info@michaelkrahn.de',
    'from_name'       => 'Michael Krahn Portfolio',

    // Automatische Bestätigungsmail (Auto-Responder) an den Anfragenden senden?
    'send_confirmation' => true,

    // SMTP-Modus (true = direkter Netcup SMTP-Login, false = Netcup PHP mail() mit Fallback)
    'use_smtp'        => false,
    'smtp_host'       => 'mx980d.netcup.net',     // Netcup Mailserver (oder mail.michaelkrahn.de)
    'smtp_port'       => 587,                    // 587 (STARTTLS) oder 465 (SSL/TLS)
    'smtp_encryption' => 'tls',                  // 'tls' (für Port 587) oder 'ssl' (für Port 465)
    'smtp_user'       => 'info@michaelkrahn.de',
    'smtp_pass'       => 'HIER_DEIN_NETCUP_MAIL_PASSWORT_EINTRAGEN',

    // Sicherheit & Spamschutz
    'allowed_origins' => [
        'https://michaelkrahn.de',
        'https://www.michaelkrahn.de',
        'https://kitchenaid.michaelkrahn.de',
        'https://schildwerk.michaelkrahn.de',
        'https://smartplanter.michaelkrahn.de',
        'https://greifarm.michaelkrahn.de',
        'https://clawmachine.michaelkrahn.de',
        'http://localhost',
        'http://127.0.0.1'
    ],
    'rate_limit_seconds' => 15, // Mindestabstand zwischen zwei Anfragen derselben IP

    // Admin-Authentifizierung (Passwort-Hash aus Umgebungsvariable ADMIN_PASSWORD_HASH oder Standard-PIN "2026")
    'admin_password_hash' => getenv('ADMIN_PASSWORD_HASH') ?: password_hash('2026', PASSWORD_DEFAULT),
    'admin_max_attempts'  => 5,
    'admin_lockout_seconds' => 300,
];

