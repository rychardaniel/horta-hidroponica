package com.hortahidroponica.model;

import io.quarkus.hibernate.orm.panache.PanacheEntityBase;
import jakarta.persistence.*;
import org.hibernate.annotations.CreationTimestamp;

import java.math.BigDecimal;
import java.time.LocalDateTime;

@Entity
@Table(name = "\"REGISTRO\"")
public class Registro extends PanacheEntityBase {

    @Id
    @GeneratedValue(strategy = GenerationType.IDENTITY)
    @Column(name = "\"ID\"")
    public Integer id;

    @CreationTimestamp
    @Column(name = "\"DATA_REGISTRO\"", nullable = false, updatable = false)
    public LocalDateTime dataRegistro;

    @Column(name = "\"PH\"", precision = 4, scale = 2)
    public BigDecimal ph;

    @Column(name = "\"CONDUTIVIDADE\"", precision = 10, scale = 2)
    public BigDecimal condutividade;

    @Column(name = "\"TEMPERATURA\"", precision = 5, scale = 2)
    public BigDecimal temperatura;

    @Column(name = "\"NIVEL_AGUA\"", precision = 5, scale = 2)
    public BigDecimal nivelAgua;

    @ManyToOne(fetch = FetchType.LAZY)
    @JoinColumn(name = "\"ID_HORTA\"", nullable = false)
    public Horta horta;

    @ManyToOne(fetch = FetchType.LAZY)
    @JoinColumn(name = "\"ID_PARAMETRO\"", nullable = false)
    public Parametro parametro;

    @ManyToOne(fetch = FetchType.LAZY)
    @JoinColumn(name = "\"ID_CULTURA\"", nullable = false)
    public Cultura cultura;
}
