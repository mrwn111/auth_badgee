from fastapi import FastAPI, Depends, HTTPException, status, Security # type: ignore
from fastapi.security import APIKeyHeader # type: ignore
from sqlalchemy import create_engine, Column, Integer, String, DateTime, exc # type: ignore
from sqlalchemy.ext.declarative import declarative_base # type: ignore
from sqlalchemy.orm import sessionmaker, Session # type: ignore
from pydantic import BaseModel, Field, validator, ConfigDict # type: ignore
from passlib.context import CryptContext # type: ignore
from datetime import datetime
import traceback
import logging

# Configuration de logging
logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)

# Configuration de sécurité - Token fixe pour l'API
API_KEY = "1a2b3c4d5e6f7g8h9i0j1k2l3m4n5o6p7q8r9s0t"
API_KEY_NAME = "X-API-Key"

# Configuration de la base de données
DATABASE_URL = "sqlite:///./badge_management.db"
engine = create_engine(DATABASE_URL, echo=True)
SessionLocal = sessionmaker(autocommit=False, autoflush=False, bind=engine)
Base = declarative_base()

# Modèle de base de données pour les utilisateurs
class User(Base):
    __tablename__ = "users"
    
    id = Column(Integer, primary_key=True, index=True)
    badge_id = Column(String, unique=True, index=True, nullable=False)
    username = Column(String, unique=True, nullable=False)
    role = Column(String, nullable=False)
    hashed_password = Column(String, nullable=False)
    created_at = Column(DateTime, default=datetime.utcnow)
    updated_at = Column(DateTime, default=datetime.utcnow, onupdate=datetime.utcnow)

# Créer les tables
try:
    Base.metadata.create_all(bind=engine)
except Exception as e:
    logger.error(f"Erreur lors de la création des tables : {e}")
    traceback.print_exc()

# Modèles Pydantic pour la validation
class UserCreate(BaseModel):
    badge_id: str = Field(..., min_length=3, max_length=50)
    password: str = Field(..., min_length=6)
    role: str = Field(..., pattern="^(admin|user)$")

    @validator('badge_id')
    def badge_id_unique(cls, v):
        return v

    model_config = ConfigDict(from_attributes=True)

class UserUpdate(BaseModel):
    role: str = Field(None, pattern="^(admin|user)$")

    model_config = ConfigDict(from_attributes=True)

class UserResponse(BaseModel):
    badge_id: str
    username: str
    role: str
    created_at: datetime
    updated_at: datetime

    model_config = ConfigDict(from_attributes=True)

# Services de sécurité
pwd_context = CryptContext(schemes=["bcrypt"], deprecated="auto")

# Sécurité avec token fixe
api_key_header = APIKeyHeader(name=API_KEY_NAME, auto_error=False)

# Dépendances
def get_db():
    db = SessionLocal()
    try:
        yield db
    finally:
        db.close()

def get_password_hash(password: str) -> str:
    return pwd_context.hash(password)

def verify_password(plain_password: str, hashed_password: str) -> bool:
    return pwd_context.verify(plain_password, hashed_password)

# Fonction corrigée pour la vérification de l'API Key
async def get_api_key(api_key_header: str = Security(api_key_header)):
    if not api_key_header:
        raise HTTPException(
            status_code=status.HTTP_401_UNAUTHORIZED,
            detail="Clé API manquante",
            headers={"WWW-Authenticate": "API key"},
        )
    
    if api_key_header != API_KEY:
        raise HTTPException(
            status_code=status.HTTP_401_UNAUTHORIZED,
            detail="Clé API invalide",
            headers={"WWW-Authenticate": "API key"},
        )
    
    return api_key_header

# Services de gestion des utilisateurs
def get_user_by_badge(db: Session, badge_id: str):
    return db.query(User).filter(User.badge_id == badge_id).first()

def get_user_by_username(db: Session, username: str):
    return db.query(User).filter(User.username == username).first()

# Application FastAPI
app = FastAPI(title="Badge Management System")

# Routes de gestion des utilisateurs
@app.post("/users", response_model=UserResponse)
async def create_user(
    user: UserCreate, 
    db: Session = Depends(get_db),
    api_key: str = Depends(get_api_key)
):
    try:
        # Vérifier si le badge ou le username existe déjà
        existing_badge = get_user_by_badge(db, user.badge_id)
        if existing_badge:
            raise HTTPException(status_code=400, detail="Badge already registered")
        
        existing_username = get_user_by_username(db, user.badge_id)
        if existing_username:
            raise HTTPException(status_code=400, detail="Username already exists")
        
        # Créer le nouvel utilisateur
        hashed_password = get_password_hash(user.password)
        db_user = User(
            badge_id=user.badge_id,
            username=user.badge_id,  # Le username est l'ID du badge
            role=user.role,
            hashed_password=hashed_password
        )
        
        db.add(db_user)
        db.commit()
        db.refresh(db_user)
        
        return db_user
    
    except exc.SQLAlchemyError as e:
        # Rollback en cas d'erreur de base de données
        db.rollback()
        logger.error(f"Erreur de base de données lors de la création d'utilisateur : {e}")
        raise HTTPException(status_code=500, detail=f"Erreur de base de données : {str(e)}")
    
    except Exception as e:
        # Capturer toute autre erreur inattendue
        logger.error(f"Erreur lors de la création d'utilisateur : {e}")
        traceback.print_exc()
        raise HTTPException(status_code=500, detail=f"Erreur interne : {str(e)}")

@app.delete("/users/{badge_id}", response_model=UserResponse)
async def delete_user(
    badge_id: str, 
    db: Session = Depends(get_db),
    api_key: str = Depends(get_api_key)
):
    try:
        # Vérifier si l'utilisateur existe
        user = get_user_by_badge(db, badge_id)
        if not user:
            raise HTTPException(status_code=404, detail="User not found")
        
        # Supprimer l'utilisateur
        db.delete(user)
        db.commit()
        
        return user
    
    except exc.SQLAlchemyError as e:
        db.rollback()
        logger.error(f"Erreur de base de données lors de la suppression d'utilisateur : {e}")
        raise HTTPException(status_code=500, detail=f"Erreur de base de données : {str(e)}")
    
    except Exception as e:
        logger.error(f"Erreur lors de la suppression d'utilisateur : {e}")
        traceback.print_exc()
        raise HTTPException(status_code=500, detail=f"Erreur interne : {str(e)}")

@app.put("/users/{badge_id}", response_model=UserResponse)
async def update_user(
    badge_id: str, 
    user_update: UserUpdate, 
    db: Session = Depends(get_db),
    api_key: str = Depends(get_api_key)
):
    try:
        # Vérifier si l'utilisateur existe
        user = get_user_by_badge(db, badge_id)
        if not user:
            raise HTTPException(status_code=404, detail="User not found")
        
        # Mettre à jour le rôle si fourni
        if user_update.role:
            user.role = user_update.role
        
        db.commit()
        db.refresh(user)
        
        return user
    
    except exc.SQLAlchemyError as e:
        db.rollback()
        logger.error(f"Erreur de base de données lors de la mise à jour d'utilisateur : {e}")
        raise HTTPException(status_code=500, detail=f"Erreur de base de données : {str(e)}")
    
    except Exception as e:
        logger.error(f"Erreur lors de la mise à jour d'utilisateur : {e}")
        traceback.print_exc()
        raise HTTPException(status_code=500, detail=f"Erreur interne : {str(e)}")

@app.get("/users/check/{badge_id}")
async def check_user_exists(
    badge_id: str, 
    db: Session = Depends(get_db),
    api_key: str = Depends(get_api_key)
):
    try:
        # Vérifier si le badge existe
        user = get_user_by_badge(db, badge_id)
        
        return {
            "exists": user is not None,
            "badge_id": badge_id,
            "user_info": UserResponse.model_validate(user) if user else None
        }
    
    except exc.SQLAlchemyError as e:
        logger.error(f"Erreur de base de données lors de la vérification du badge : {e}")
        raise HTTPException(status_code=500, detail=f"Erreur de base de données : {str(e)}")
    
    except Exception as e:
        logger.error(f"Erreur lors de la vérification du badge : {e}")
        traceback.print_exc()
        raise HTTPException(status_code=500, detail=f"Erreur interne : {str(e)}")

# Exemple d'endpoint protégé qui nécessite une clé API valide
@app.get("/users/all", response_model=list[UserResponse])
async def get_all_users(
    db: Session = Depends(get_db),
    api_key: str = Depends(get_api_key)
):
    try:
        users = db.query(User).all()
        return users
    except Exception as e:
        logger.error(f"Erreur lors de la récupération des utilisateurs : {e}")
        raise HTTPException(status_code=500, detail=f"Erreur interne : {str(e)}")

# Route pour tester que l'API Key fonctionne correctement
@app.get("/test-api-key")
async def test_api_key(api_key: str = Depends(get_api_key)):
    return {"message": "API Key valide", "status": "success"}

if __name__ == "__main__":
    import uvicorn # type: ignore
    uvicorn.run(app, host="0.0.0.0", port=8000)