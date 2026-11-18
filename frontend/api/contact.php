<?php
/**
 * ============================================================================
 * PORTFOLIO & MANUFAKTUR KONTAKT-API FÜR NETCUP WEBHOSTING
 * Unterstützt sowohl nativen Netcup SMTP-Versand (STARTTLS/SSL) als auch PHP mail()
 * ============================================================================
 */

header('Content-Type: application/json; charset=utf-8');

// Load configuration
$configFile = __DIR__ . '/mail_config.php';
if (!file_exists($configFile)) {
    http_response_code(500);
    echo json_encode(['success' => false, 'error' => 'Server configuration missing.']);
    exit;
}
$config = require $configFile;

// Handle CORS for local testing & subdomains
$origin = $_SERVER['HTTP_ORIGIN'] ?? '';
if ($origin) {
    foreach ($config['allowed_origins'] as $allowed) {
        if (strpos($origin, $allowed) === 0) {
            header("Access-Control-Allow-Origin: $origin");
            header("Access-Control-Allow-Methods: POST, OPTIONS");
            header("Access-Control-Allow-Headers: Content-Type, X-Requested-With");
            break;
        }
    }
}

if ($_SERVER['REQUEST_METHOD'] === 'OPTIONS') {
    http_response_code(204);
    exit;
}

if ($_SERVER['REQUEST_METHOD'] !== 'POST') {
    http_response_code(405);
    echo json_encode(['success' => false, 'error' => 'Method not allowed. Use POST.']);
    exit;
}

// Parse JSON or FormData payload
$rawBody = file_get_contents('php://input');
$data = json_decode($rawBody, true);
if (!is_array($data)) {
    $data = $_POST;
}

// 1. Honeypot Spam Protection (if hidden field "website_url" is filled -> bot)
if (!empty($data['website_url'])) {
    // Pretend success to fool bots
    echo json_encode(['success' => true, 'message' => 'Message sent.']);
    exit;
}

// 2. Simple Session/File-based Rate Limiting
session_start();
$now = time();
$rateLimit = $config['rate_limit_seconds'] ?? 15;
if (isset($_SESSION['last_contact_submit']) && ($now - $_SESSION['last_contact_submit']) < $rateLimit) {
    http_response_code(429);
    echo json_encode([
        'success' => false,
        'error'   => 'Bitte warte einen kurzen Moment vor dem nächsten Senden. / Please wait a moment before sending again.'
    ]);
    exit;
}

// 3. Extract & Sanitize Fields
$name     = trim(strip_tags($data['name'] ?? ''));
$email    = trim(filter_var($data['email'] ?? '', FILTER_SANITIZE_EMAIL));
$subject  = trim(strip_tags($data['subject'] ?? 'Neue Portfolio-Anfrage'));
$category = trim(strip_tags($data['category'] ?? 'Allgemein'));
$message  = trim(strip_tags($data['message'] ?? ''));
$source   = preg_replace('/[^a-zA-Z0-9_-]/', '', trim((string)($data['source'] ?? 'portfolio'))) ?: 'portfolio';
$lang     = ($data['lang'] ?? 'de') === 'en' ? 'en' : 'de';

// Prevent header injection in name/subject
$name     = preg_replace('/[\r\n]+/', ' ', $name);
$subject  = preg_replace('/[\r\n]+/', ' ', $subject);
$category = preg_replace('/[\r\n]+/', ' ', $category);

// Validate required fields
if (empty($name) || empty($email) || empty($message)) {
    http_response_code(400);
    echo json_encode([
        'success' => false,
        'error'   => $lang === 'en'
            ? 'Please fill in all required fields (Name, Email, Message).'
            : 'Bitte fülle alle Pflichtfelder (Name, E-Mail, Nachricht) aus.'
    ]);
    exit;
}

if (!filter_var($email, FILTER_VALIDATE_EMAIL)) {
    http_response_code(400);
    echo json_encode([
        'success' => false,
        'error'   => $lang === 'en'
            ? 'Please enter a valid email address.'
            : 'Bitte gib eine gültige E-Mail-Adresse ein.'
    ]);
    exit;
}

// 4. Build Email Content for Michael
$mailSubject = "[Portfolio & Manufaktur] " . ($subject ?: $category) . " – von $name";

$htmlBody = '<!DOCTYPE html>
<html lang="de">
<head><meta charset="UTF-8"></head>
<body style="font-family: Inter, -apple-system, BlinkMacSystemFont, sans-serif; background:#f4f5f7; padding:24px; color:#0f1115;">
  <div style="max-width:600px; margin:0 auto; background:#ffffff; border-radius:16px; padding:28px; box-shadow:0 8px 24px rgba(0,0,0,0.08); border:1px solid #e5e7eb;">
    <div style="font-size:12px; text-transform:uppercase; letter-spacing:0.12em; color:#64748b; margin-bottom:6px;">
      Michael Krahn • Creative Studio &amp; Manufaktur
    </div>
    <h2 style="margin:0 0 20px 0; font-size:22px; color:#0f1115;">Neue Projekt- / Kontaktanfrage</h2>
    <table style="width:100%; border-collapse:collapse; margin-bottom:20px; font-size:14px;">
      <tr>
        <td style="padding:8px 0; color:#64748b; width:130px;"><strong>Name:</strong></td>
        <td style="padding:8px 0; color:#0f1115;">' . htmlspecialchars($name, ENT_QUOTES, 'UTF-8') . '</td>
      </tr>
      <tr>
        <td style="padding:8px 0; color:#64748b;"><strong>E-Mail:</strong></td>
        <td style="padding:8px 0;"><a href="mailto:' . htmlspecialchars($email, ENT_QUOTES, 'UTF-8') . '" style="color:#2563eb;">' . htmlspecialchars($email, ENT_QUOTES, 'UTF-8') . '</a></td>
      </tr>
      <tr>
        <td style="padding:8px 0; color:#64748b;"><strong>Kategorie-Chip:</strong></td>
        <td style="padding:8px 0;"><span style="background:#e0f2fe; color:#0369a1; padding:3px 10px; border-radius:999px; font-weight:600; font-size:12px;">' . htmlspecialchars($category, ENT_QUOTES, 'UTF-8') . '</span></td>
      </tr>
      <tr>
        <td style="padding:8px 0; color:#64748b;"><strong>Betreff:</strong></td>
        <td style="padding:8px 0; font-weight:600; color:#0f1115;">' . htmlspecialchars($subject, ENT_QUOTES, 'UTF-8') . '</td>
      </tr>
    </table>
    <div style="background:#f8fafc; border-left:4px solid #38bdf8; padding:16px; border-radius:8px; font-size:15px; line-height:1.6; white-space:pre-wrap;">' . htmlspecialchars($message, ENT_QUOTES, 'UTF-8') . '</div>
    <div style="margin-top:20px; font-size:12px; color:#94a3b8;">
      Gesendet am ' . date('d.m.Y \u\m H:i:s') . ' Uhr über dein Netcup Webhosting Kontaktformular. Antworte einfach direkt auf diese E-Mail (Reply-To ist gesetzt).
    </div>
  </div>
</body>
</html>';

$plainBody = "Neue Anfrage über michaelkrahn.de\n"
           . "====================================\n"
           . "Name:      $name\n"
           . "E-Mail:    $email\n"
           . "Kategorie: $category\n"
           . "Betreff:   $subject\n"
           . "------------------------------------\n\n"
           . $message . "\n\n"
           . "------------------------------------\n"
           . "Gesendet am " . date('d.m.Y H:i:s');

// 5. Send Email Function (Supports Netcup SMTP + native PHP mail() fallback)
function sendNetcupEmail(array $cfg, string $toEmail, string $toName, string $replyEmail, string $replyName, string $subject, string $html, string $plain): array {
    $fromEmail = $cfg['from_email'];
    $fromName  = $cfg['from_name'];
    $boundary  = '=_Part_' . md5(uniqid((string)microtime(true), true));

    $headers = [
        'MIME-Version: 1.0',
        'From: ' . mb_encode_mimeheader($fromName, 'UTF-8') . " <{$fromEmail}>",
        'Reply-To: ' . mb_encode_mimeheader($replyName, 'UTF-8') . " <{$replyEmail}>",
        'Date: ' . date('r'),
        'Message-ID: <' . uniqid() . '@' . ($_SERVER['HTTP_HOST'] ?? 'michaelkrahn.de') . '>',
        'X-Mailer: MichaelKrahn-Portfolio-Netcup/2.0',
        "Content-Type: multipart/alternative; boundary=\"{$boundary}\""
    ];

    $mimeBody = "--{$boundary}\r\n"
              . "Content-Type: text/plain; charset=UTF-8\r\n"
              . "Content-Transfer-Encoding: base64\r\n\r\n"
              . chunk_split(base64_encode($plain)) . "\r\n"
              . "--{$boundary}\r\n"
              . "Content-Type: text/html; charset=UTF-8\r\n"
              . "Content-Transfer-Encoding: base64\r\n\r\n"
              . chunk_split(base64_encode($html)) . "\r\n"
              . "--{$boundary}--\r\n";

    // Option A: Authenticated SMTP (if enabled and password configured)
    if (!empty($cfg['use_smtp']) && $cfg['smtp_pass'] !== 'HIER_DEIN_NETCUP_MAIL_PASSWORT_EINTRAGEN') {
        $smtpResult = sendViaSmtpSocket($cfg, $fromEmail, $toEmail, $subject, $headers, $mimeBody);
        if ($smtpResult['success']) {
            return $smtpResult;
        }
        // If SMTP fails, log and try native PHP mail() fallback
        error_log('Netcup SMTP failed, falling back to mail(): ' . ($smtpResult['error'] ?? ''));
    }

    // Option B: Netcup native PHP mail()
    $encodedSubject = mb_encode_mimeheader($subject, 'UTF-8');
    $headerString   = implode("\r\n", $headers);
    $additionalParams = "-f" . escapeshellarg($fromEmail);

    $ok = @mail($toEmail, $encodedSubject, $mimeBody, $headerString, $additionalParams);
    if (!$ok) {
        // Fallback without -f parameter in restricted environments
        $ok = @mail($toEmail, $encodedSubject, $mimeBody, $headerString);
    }

    return [
        'success' => (bool)$ok,
        'error'   => $ok ? null : 'PHP mail() returned false. Please check mail_config.php on your Netcup server.'
    ];
}

/**
 * Lightweight zero-dependency SMTP client for Netcup Mailserver (STARTTLS / SSL)
 */
function sendViaSmtpSocket(array $cfg, string $from, string $to, string $subject, array $headers, string $body): array {
    $host = $cfg['smtp_host'];
    $port = (int)($cfg['smtp_port'] ?? 587);
    $enc  = strtolower($cfg['smtp_encryption'] ?? 'tls');
    $user = $cfg['smtp_user'];
    $pass = $cfg['smtp_pass'];

    $remote = ($enc === 'ssl' ? "ssl://{$host}:{$port}" : "tcp://{$host}:{$port}");
    $ctx = stream_context_create([
        'ssl' => [
            'verify_peer'       => true,
            'verify_peer_name'  => true,
            'allow_self_signed' => false
        ]
    ]);

    $fp = @stream_socket_client($remote, $errno, $errstr, 10, STREAM_CLIENT_CONNECT, $ctx);
    if (!$fp) {
        return ['success' => false, 'error' => "SMTP connect error ($errno): $errstr"];
    }
    stream_set_timeout($fp, 10);

    $read = function() use ($fp) {
        $data = '';
        while ($str = fgets($fp, 515)) {
            $data .= $str;
            if (substr($str, 3, 1) === ' ') break;
        }
        return $data;
    };

    $cmd = function(string $command, int $expectCode) use ($fp, $read) {
        fwrite($fp, $command . "\r\n");
        $resp = $read();
        $code = (int)substr($resp, 0, 3);
        return [$code === $expectCode, $resp];
    };

    $greeting = $read();
    if ((int)substr($greeting, 0, 3) !== 220) {
        fclose($fp);
        return ['success' => false, 'error' => "SMTP greeting failed: $greeting"];
    }

    $ehloHost = $_SERVER['HTTP_HOST'] ?? 'michaelkrahn.de';
    [$ok, $resp] = $cmd("EHLO {$ehloHost}", 250);
    if (!$ok) { fclose($fp); return ['success' => false, 'error' => "EHLO failed: $resp"]; }

    if ($enc === 'tls') {
        [$ok, $resp] = $cmd("STARTTLS", 220);
        if (!$ok) { fclose($fp); return ['success' => false, 'error' => "STARTTLS failed: $resp"]; }
        if (!stream_socket_enable_crypto($fp, true, STREAM_CRYPTO_METHOD_TLSv1_2_CLIENT | STREAM_CRYPTO_METHOD_TLSv1_3_CLIENT)) {
            fclose($fp);
            return ['success' => false, 'error' => "TLS negotiation failed"];
        }
        [$ok, $resp] = $cmd("EHLO {$ehloHost}", 250);
        if (!$ok) { fclose($fp); return ['success' => false, 'error' => "EHLO after TLS failed: $resp"]; }
    }

    [$ok, $resp] = $cmd("AUTH LOGIN", 334);
    if (!$ok) { fclose($fp); return ['success' => false, 'error' => "AUTH LOGIN failed: $resp"]; }

    [$ok, $resp] = $cmd(base64_encode($user), 334);
    if (!$ok) { fclose($fp); return ['success' => false, 'error' => "SMTP username rejected: $resp"]; }

    [$ok, $resp] = $cmd(base64_encode($pass), 235);
    if (!$ok) { fclose($fp); return ['success' => false, 'error' => "SMTP password rejected: $resp"]; }

    [$ok, $resp] = $cmd("MAIL FROM:<{$from}>", 250);
    if (!$ok) { fclose($fp); return ['success' => false, 'error' => "MAIL FROM failed: $resp"]; }

    [$ok, $resp] = $cmd("RCPT TO:<{$to}>", 250);
    if (!$ok) { fclose($fp); return ['success' => false, 'error' => "RCPT TO failed: $resp"]; }

    [$ok, $resp] = $cmd("DATA", 354);
    if (!$ok) { fclose($fp); return ['success' => false, 'error' => "DATA command failed: $resp"]; }

    $fullMsg = "To: <{$to}>\r\n"
             . "Subject: " . mb_encode_mimeheader($subject, 'UTF-8') . "\r\n"
             . implode("\r\n", $headers) . "\r\n\r\n"
             . $body . "\r\n.";

    [$ok, $resp] = $cmd($fullMsg, 250);
    $cmd("QUIT", 221);
    fclose($fp);

    return ['success' => $ok, 'error' => $ok ? null : "DATA send failed: $resp"];
}

// 4.5 Persist Inquiry to JSON Storage (for Website Mail/Inquiry Manager)
$storageDir = __DIR__ . '/../data/storage';
if (!is_dir($storageDir)) {
    @mkdir($storageDir, 0755, true);
}
$htaccessFile = $storageDir . '/.htaccess';
if (!file_exists($htaccessFile)) {
    @file_put_contents($htaccessFile, "Require all denied\nDeny from all\n");
}
$inquiriesFile = $storageDir . '/inquiries.json';
$inquiries = [];
if (file_exists($inquiriesFile)) {
    $rawInq = @file_get_contents($inquiriesFile);
    $decodedInq = $rawInq ? json_decode($rawInq, true) : null;
    if (is_array($decodedInq)) {
        $inquiries = $decodedInq;
    }
}
$inquiryId = 'INQ-' . date('Ymd-His') . '-' . strtoupper(substr(md5(uniqid((string)microtime(true), true)), 0, 4));
$newInquiry = [
    'id'         => $inquiryId,
    'createdAt'  => date('c'),
    'updatedAt'  => date('c'),
    'source'     => $source,
    'name'       => $name,
    'email'      => $email,
    'subject'    => $subject,
    'category'   => $category,
    'message'    => $message,
    'lang'       => $lang,
    'status'     => 'open',
    'notes'      => '',
    'emailSent'  => false,
    'emailError' => null,
    'replies'    => [],
];
array_unshift($inquiries, $newInquiry);
@file_put_contents($inquiriesFile, json_encode($inquiries, JSON_PRETTY_PRINT | JSON_UNESCAPED_UNICODE));

// Send main inquiry email to Michael
$result = sendNetcupEmail(
    $config,
    $config['recipient_email'],
    $config['recipient_name'],
    $email,
    $name,
    $mailSubject,
    $htmlBody,
    $plainBody
);

// Update email delivery status in inquiries.json
foreach ($inquiries as &$inq) {
    if (($inq['id'] ?? '') === $inquiryId) {
        $inq['emailSent']  = (bool)$result['success'];
        $inq['emailError'] = $result['error'] ?? null;
        $inq['updatedAt']  = date('c');
        break;
    }
}
unset($inq);
@file_put_contents($inquiriesFile, json_encode($inquiries, JSON_PRETTY_PRINT | JSON_UNESCAPED_UNICODE));

if (!$result['success']) {
    http_response_code(500);
    echo json_encode([
        'success' => false,
        'error'   => $lang === 'en'
            ? 'Could not send message via mail server. Please try again or email directly.'
            : 'Nachricht konnte nicht über den Mailserver gesendet werden. Bitte versuche es erneut oder schreibe direkt per E-Mail.',
        'debug'   => $result['error']
    ]);
    exit;
}

// Optional: Send Auto-Responder Confirmation to Visitor
if (!empty($config['send_confirmation'])) {
    $confirmSubject = $lang === 'en'
        ? "Confirmation: Your inquiry to Michael Krahn (" . ($subject ?: $category) . ")"
        : "Bestätigung: Deine Anfrage bei Michael Krahn (" . ($subject ?: $category) . ")";

    $confirmPlain = $lang === 'en'
        ? "Hi $name,\n\nthank you for your message regarding \"$subject\"!\nI have received your inquiry and will get back to you personally as soon as possible.\n\nYour message:\n$message\n\nBest regards,\nMichael Krahn\nCreative Developer & Maker Studio\nhttps://michaelkrahn.de"
        : "Hallo $name,\n\nvielen Dank für deine Nachricht zum Thema \"$subject\"!\nIch habe deine Anfrage erhalten und melde mich schnellstmöglich persönlich bei dir zurück.\n\nDeine Nachricht:\n$message\n\nBeste Grüße\nMichael Krahn\nCreative Developer & Maker Studio\nhttps://michaelkrahn.de";

    $confirmHtml = '<!DOCTYPE html><html><body style="font-family:Inter,sans-serif; color:#0f1115; background:#f4f5f7; padding:24px;">'
        . '<div style="max-width:560px; margin:0 auto; background:#fff; padding:28px; border-radius:16px; border:1px solid #e5e7eb;">'
        . '<h2 style="margin-top:0;">' . ($lang === 'en' ? "Thanks for reaching out, " : "Danke für deine Anfrage, ") . htmlspecialchars($name, ENT_QUOTES, 'UTF-8') . '!</h2>'
        . '<p style="line-height:1.6; color:#334155;">'
        . ($lang === 'en'
            ? 'I have received your message regarding <strong>' . htmlspecialchars($subject, ENT_QUOTES, 'UTF-8') . '</strong> and will get back to you soon.'
            : 'Ich habe deine Nachricht zum Thema <strong>' . htmlspecialchars($subject, ENT_QUOTES, 'UTF-8') . '</strong> erhalten und melde mich zeitnah persönlich bei dir.')
        . '</p>'
        . '<div style="background:#f8fafc; border-left:3px solid #38bdf8; padding:14px; border-radius:8px; font-size:14px; color:#475569; white-space:pre-wrap;">'
        . htmlspecialchars($message, ENT_QUOTES, 'UTF-8')
        . '</div>'
        . '<p style="margin-top:20px; font-size:14px; color:#64748b;">Beste Grüße / Best regards,<br><strong style="color:#0f1115;">Michael Krahn</strong><br>Maker &amp; Creative Developer</p>'
        . '</div></body></html>';

    @sendNetcupEmail(
        $config,
        $email,
        $name,
        $config['from_email'],
        $config['recipient_name'],
        $confirmSubject,
        $confirmHtml,
        $confirmPlain
    );
}

$_SESSION['last_contact_submit'] = $now;

echo json_encode([
    'success' => true,
    'message' => $lang === 'en'
        ? 'Message sent successfully! A confirmation has been sent to your email address.'
        : 'Nachricht erfolgreich gesendet! Eine Bestätigung wurde an deine E-Mail-Adresse geschickt.'
]);

