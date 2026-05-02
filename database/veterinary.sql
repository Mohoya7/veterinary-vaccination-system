CREATE DATABASE IF NOT EXISTS veterinary
    CHARACTER SET utf8mb4
    COLLATE utf8mb4_persian_ci;

USE veterinary;

-- -----------------------------------------------
-- users table
-- -----------------------------------------------
CREATE TABLE users (
    id            INT PRIMARY KEY AUTO_INCREMENT,
    username      VARCHAR(50) UNIQUE NOT NULL,
    password_hash VARCHAR(255) NOT NULL,
    role          ENUM('admin', 'technician') NOT NULL,
    is_active     BOOLEAN DEFAULT TRUE,
    created_at    DATETIME DEFAULT NOW(),
    updated_at    DATETIME
);

INSERT INTO users (username, password_hash, role, is_active) VALUES
('admin',      SHA2('admin123', 256), 'admin',      TRUE),
('technician', SHA2('tech123',  256), 'technician', TRUE);

-- -----------------------------------------------
-- owners table
-- -----------------------------------------------
CREATE TABLE owners (
    id              INT PRIMARY KEY AUTO_INCREMENT,
    first_name      VARCHAR(100) NOT NULL,
    last_name       VARCHAR(100) NOT NULL,
    phone           VARCHAR(20) NOT NULL,
    phone_secondary VARCHAR(20),
    address         TEXT,
    notes           TEXT,
    is_deleted      BOOLEAN DEFAULT FALSE,
    created_at      DATETIME DEFAULT NOW(),
    updated_at      DATETIME
);

-- -----------------------------------------------
-- animals table
-- -----------------------------------------------
CREATE TABLE animals (
    id          INT PRIMARY KEY AUTO_INCREMENT,
    name        VARCHAR(100) NOT NULL,
    type        ENUM('dog', 'cat') NOT NULL,
    breed       VARCHAR(100),
    birth_date  DATE,
    gender      ENUM('male', 'female') NOT NULL,
    weight      DECIMAL(5,2),
    owner_id    INT NOT NULL,
    is_deleted  BOOLEAN DEFAULT FALSE,
    created_at  DATETIME DEFAULT NOW(),
    updated_at  DATETIME,
    FOREIGN KEY (owner_id) REFERENCES owners(id)
);

-- -----------------------------------------------
-- animal_weights table
-- -----------------------------------------------
CREATE TABLE animal_weights (
    id          INT PRIMARY KEY AUTO_INCREMENT,
    animal_id   INT NOT NULL,
    weight      DECIMAL(5,2) NOT NULL,
    recorded_at DATE NOT NULL,
    notes       TEXT,
    FOREIGN KEY (animal_id) REFERENCES animals(id)
);

-- -----------------------------------------------
-- vaccine_types table
-- -----------------------------------------------
CREATE TABLE vaccine_types (
    id                    INT PRIMARY KEY AUTO_INCREMENT,
    name                  VARCHAR(100) NOT NULL,
    animal_type           ENUM('dog', 'cat', 'both') NOT NULL,
    default_reminder_days INT NOT NULL,
    is_active             BOOLEAN DEFAULT TRUE
);

INSERT INTO vaccine_types (name, animal_type, default_reminder_days) VALUES
('واکسن ۸ گانه', 'dog',  365),
('واکسن ۴ گانه', 'cat',  365),
('واکسن هاری',   'both', 365),
('قرص ضد انگل',  'both', 90);

-- -----------------------------------------------
-- vaccinations table
-- -----------------------------------------------
CREATE TABLE vaccinations (
    id               INT PRIMARY KEY AUTO_INCREMENT,
    animal_id        INT NOT NULL,
    vaccine_type_id  INT NOT NULL,
    vaccinated_at    DATE NOT NULL,
    reminder_days    INT NOT NULL,
    next_reminder_at DATE NOT NULL,
    notes            TEXT,
    is_deleted       BOOLEAN DEFAULT FALSE,
    created_at       DATETIME DEFAULT NOW(),
    updated_at       DATETIME,
    FOREIGN KEY (animal_id)       REFERENCES animals(id),
    FOREIGN KEY (vaccine_type_id) REFERENCES vaccine_types(id)
);

-- -----------------------------------------------
-- reminder_followups table
-- -----------------------------------------------
CREATE TABLE reminder_followups (
    id              INT PRIMARY KEY AUTO_INCREMENT,
    vaccination_id  INT NOT NULL,
    sms_sent        BOOLEAN DEFAULT FALSE,
    sms_sent_at     DATETIME,
    is_followed_up  BOOLEAN DEFAULT FALSE,
    followed_up_at  DATETIME,
    owner_responded BOOLEAN DEFAULT FALSE,
    is_resolved     BOOLEAN DEFAULT FALSE,
    notes           TEXT,
    created_at      DATETIME DEFAULT NOW(),
    FOREIGN KEY (vaccination_id) REFERENCES vaccinations(id)
);

-- -----------------------------------------------
-- backup_settings table
-- -----------------------------------------------
CREATE TABLE backup_settings (
    id              INT PRIMARY KEY,
    backup_path     VARCHAR(500),
    interval_days   INT DEFAULT 1,
    is_auto_enabled BOOLEAN DEFAULT TRUE,
    last_backup_at  DATETIME,
    updated_at      DATETIME
);

INSERT INTO backup_settings (id) VALUES (1);

-- -----------------------------------------------
-- sms_settings table
-- -----------------------------------------------
CREATE TABLE sms_settings (
    id            INT PRIMARY KEY,
    api_key       VARCHAR(255),
    api_url       VARCHAR(500),
    sender_number VARCHAR(20),
    is_enabled    BOOLEAN DEFAULT FALSE,
    updated_at    DATETIME
);

INSERT INTO sms_settings (id) VALUES (1);