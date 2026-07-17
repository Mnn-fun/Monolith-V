-- ==============================================================================
-- MonolithV - Version 1 Oracle Database Schema Migration Script
-- ==============================================================================
-- Target Engine: Oracle Autonomous Transaction Processing (ATP) / 19c / 23ai
-- Purpose: Month-1 foundational relational schema covering vertical slice:
--          player accounts/profiles, season state, role assignment,
--          role-item share events (Golden Apple mechanic), and checkpoint progress.
-- ==============================================================================

-- ------------------------------------------------------------------------------
-- 1. PLAYERS Table
-- Stores core player identity linked to Epic Online Services (EOS).
-- ------------------------------------------------------------------------------
CREATE TABLE players (
    player_id       VARCHAR2(64) NOT NULL,
    eos_account_id  VARCHAR2(128) NOT NULL,
    display_name    VARCHAR2(128) NOT NULL,
    created_at      TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP NOT NULL,
    CONSTRAINT pk_players PRIMARY KEY (player_id),
    CONSTRAINT uq_players_eos_account UNIQUE (eos_account_id)
);

-- ------------------------------------------------------------------------------
-- 2. SEASONS Table
-- Tracks seasonal cycles and active state across the game world.
-- ------------------------------------------------------------------------------
CREATE TABLE seasons (
    season_id       VARCHAR2(64) NOT NULL,
    season_number   NUMBER NOT NULL,
    started_at      TIMESTAMP WITH TIME ZONE NOT NULL,
    ends_at         TIMESTAMP WITH TIME ZONE,
    is_active       NUMBER(1) DEFAULT 0 NOT NULL,
    CONSTRAINT pk_seasons PRIMARY KEY (season_id),
    CONSTRAINT uq_seasons_number UNIQUE (season_number),
    CONSTRAINT chk_seasons_is_active CHECK (is_active IN (0, 1))
);

-- ------------------------------------------------------------------------------
-- 3. PLAYER_SEASON_ROLES Table
-- Enforces the "role chosen per season" rule at the database level.
-- Each player can select exactly one role ('MALE' or 'FEMALE') per season.
-- ------------------------------------------------------------------------------
CREATE TABLE player_season_roles (
    player_id       VARCHAR2(64) NOT NULL,
    season_id       VARCHAR2(64) NOT NULL,
    role            VARCHAR2(32) NOT NULL,
    CONSTRAINT pk_player_season_roles PRIMARY KEY (player_id, season_id),
    CONSTRAINT fk_psr_player FOREIGN KEY (player_id)
        REFERENCES players (player_id) ON DELETE CASCADE,
    CONSTRAINT fk_psr_season FOREIGN KEY (season_id)
        REFERENCES seasons (season_id) ON DELETE CASCADE,
    CONSTRAINT chk_psr_role CHECK (role IN ('MALE', 'FEMALE'))
);

CREATE INDEX ix_psr_player_id ON player_season_roles (player_id);
CREATE INDEX ix_psr_season_id ON player_season_roles (season_id);

-- ------------------------------------------------------------------------------
-- 4. SHARE_EVENTS Table
-- Records role-item share events (the Golden Apple / Counterpart Item mechanic).
-- This table proves a share occurred and is queried during gate checks.
-- ------------------------------------------------------------------------------
CREATE TABLE share_events (
    share_event_id      VARCHAR2(64) NOT NULL,
    season_id           VARCHAR2(64) NOT NULL,
    giver_player_id     VARCHAR2(64) NOT NULL,
    receiver_player_id  VARCHAR2(64) NOT NULL,
    item_type           VARCHAR2(64) NOT NULL,
    shared_at           TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP NOT NULL,
    CONSTRAINT pk_share_events PRIMARY KEY (share_event_id),
    CONSTRAINT fk_se_season FOREIGN KEY (season_id)
        REFERENCES seasons (season_id) ON DELETE CASCADE,
    CONSTRAINT fk_se_giver FOREIGN KEY (giver_player_id)
        REFERENCES players (player_id) ON DELETE CASCADE,
    CONSTRAINT fk_se_receiver FOREIGN KEY (receiver_player_id)
        REFERENCES players (player_id) ON DELETE CASCADE,
    CONSTRAINT chk_se_item_type CHECK (item_type IN ('GOLDEN_APPLE', 'COUNTERPART_ITEM')),
    CONSTRAINT chk_se_diff_players CHECK (giver_player_id <> receiver_player_id)
);

CREATE INDEX ix_se_season_id ON share_events (season_id);
CREATE INDEX ix_se_giver_player_id ON share_events (giver_player_id);
-- Composite lookup index explicitly required for the P2.6 gate-check query:
CREATE INDEX ix_se_receiver_season ON share_events (receiver_player_id, season_id);

-- ------------------------------------------------------------------------------
-- 5. CHECKPOINT_PROGRESS Table
-- Tracks player progression across ordered world checkpoints within a season.
-- ------------------------------------------------------------------------------
CREATE TABLE checkpoint_progress (
    player_id           VARCHAR2(64) NOT NULL,
    season_id           VARCHAR2(64) NOT NULL,
    checkpoint_index    NUMBER NOT NULL,
    reached_at          TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP NOT NULL,
    CONSTRAINT pk_checkpoint_progress PRIMARY KEY (player_id, season_id, checkpoint_index),
    CONSTRAINT fk_cp_player FOREIGN KEY (player_id)
        REFERENCES players (player_id) ON DELETE CASCADE,
    CONSTRAINT fk_cp_season FOREIGN KEY (season_id)
        REFERENCES seasons (season_id) ON DELETE CASCADE
);

CREATE INDEX ix_cp_player_id ON checkpoint_progress (player_id);
CREATE INDEX ix_cp_season_id ON checkpoint_progress (season_id);

-- ==============================================================================
-- End of V1 Schema Migration
-- ==============================================================================
